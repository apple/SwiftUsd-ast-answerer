import argparse
import re
import os
import pathlib

class _ModelBase:
    """
    Basic functionality not particularly tied to analysis
    """
    
    def __init__(self):
        self.log = []

    def _print_and_exit(self, s, error_code=1):
        print(s)
        exit(error_code)

    def _log(self, s):
        self.log.append(s + "\n")

    def _load(self, path, kinds_to_types, types_to_kinds, kinds, types):
        with open(path, "r") as f:
            lines = f.read().splitlines()

            for l in lines:
                match = re.match(r"(^.*);\s+(.+);$", l)
                if match is None:
                    self._print_and_exit(f"Illegal line {l}")


                def normalize_pxr_namespace(x):
                    x = re.sub("pxrInternal_v0_24__pxrReserved__", "PXR_NS", x)
                    x = re.sub("pxrInternal_v0_24_11__pxrReserved__", "PXR_NS", x)
                    x = re.sub("pxrInternal_v0_25_2__pxrReserved__", "PXR_NS", x)
                    x = re.sub("pxrInternal_v0_25_5__pxrReserved__", "PXR_NS", x)
                    return x

                
                type_ = normalize_pxr_namespace(match.group(1))
                kind = normalize_pxr_namespace(match.group(2))


                if kind not in kinds_to_types:
                    kinds_to_types[kind] = set()
                kinds_to_types[kind].add(type_)

                types_to_kinds[type_] = kind

                kinds.add(kind)
                types.add(type_)

    def _find_path(self, base, trait):
        for (dirpath, dirnames, filenames) in os.walk(base):
            for filename in filenames:
                if filename == trait or os.path.splitext(filename)[0] == trait:
                    return pathlib.Path(dirpath) / filename

        self._print_and_exit(f"Error: could not find {trait} under {base}")
        exit(1)

    def _meta_analysis_pass(self, name):
        self._name_for__enter__ = name + ":"
        return self

    def __enter__(self):
        self._log(self._name_for__enter__)
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self._log("")

class AtomicFilter:
    @staticmethod
    def helpText() -> str:
        return """filter            := inversion element (logical_predicate | string_predicate `:` string_argument)
inversion         := `!`?
element           := `kind.old` | `kind.new` | `type`
logical_predicate := `.is_added` | `.is_removed` | `.is_pxr` | `.is_stl` | `.changed_kind`
string_predicate  := `.is` | `.starts_with` | `.ends_with` | `.contains`
string_argument   := .*
"""
    
    def __init__(self, s):
        """
        An atomic filter that can be supplied to suppress certain results, of the form specified in AtomicFilter.helpText()
        """

        self.s_orig = s

        if not s: raise ValueError(f"Invalid filter 1: {self.s_orig}")

        self.is_inverted = s[0] == "!"
        if self.is_inverted:
            s = s[1:]
            if not s: raise ValueError(f"Invalid filter 2: {self.s_orig}")

        for element in ["type", "kind.old", "kind.new"]:
            if s.startswith(element + "."):
                self.element = element
                s = s[len(element) + 1:]
                if not s: raise ValueError(f"Invalid filter 3: {self.s_orig}")
                break

        self.logical_predicate = None
        self.string_predicate = None
        
        for logical_predicate in ["is_added", "is_removed", "is_pxr", "is_stl", "changed_kind"]:
            if s.startswith(logical_predicate):
                self.logical_predicate = logical_predicate
                s = s[len(logical_predicate):]
                if s: raise ValueError(f"Invalid filter 4: {self.s_orig}")
                break

        if self.logical_predicate is None:
            for string_predicate in ["is", "starts_with", "ends_with", "contains"]:
                if s.startswith(string_predicate + ":"):
                    self.string_predicate = string_predicate
                    s = s[1 + len(string_predicate):]
                    self.string_argument = s
                    break

            if self.string_predicate is None:
                raise ValueError(f"Invalid filter 5: {self.s_orig}")


    def run(self, old_kind, new_kind, type_) -> bool:
        result = None

        def run_string_predicate(arg) -> bool:
            if self.string_predicate == "is":
                return arg == self.string_argument
            if self.string_predicate == "starts_with":
                return arg.startswith(self.string_argument)
            if self.string_predicate == "ends_with":
                return arg.endswith(self.string_argument)
            if self.string_predicate == "contains":
                return self.string_argument in arg
        
        if self.element == "type":
            if self.logical_predicate == "is_added":
                result = old_kind is None and new_kind is not None
            elif self.logical_predicate == "is_removed":
                result = old_kind is not None and new_kind is None
            elif self.logical_predicate == "is_pxr":
                for x in ["class", "struct", "enum", "union", ""]:
                    if type_.startswith(x + " PXR_NS"):
                        result = True
                        break
                else:
                    result = False
                    
            elif self.logical_predicate == "is_stl":
                for x in ["class", "struct", "enum", "union", ""]:
                    if type_.startswith(x + " std::"):
                        result = True
                        break
                else:
                    result = False

            elif self.logical_predicate == "changed_kind":
                result = old_kind != new_kind

            else:
                result = run_string_predicate(type_)
                
        elif self.element == "kind.old":
            result = run_string_predicate(old_kind)
        
        elif self.element == "kind.new":
            result = run_string_predicate(new_kind)

        if result is None:
            raise ValueError(f"Invalid filter: {self.s_orig}, {old_kind}, {new_kind}, {type_}")

        return not result if self.is_inverted else result

class Filter:
    """
    A filter containing a series of atomic filters combined with &, |, and ()
    """
    
    def __init__(self, s=None, tokens=None,
                 left=None, right=None, combination=None):
        if left and right and combination:
            self.left = left
            self.right = right
            self.combination = combination
            return

        if s == "":
            class TrueFilter:
                def __init__(self):
                    pass
                def run(self, old_kind, new_kind, type_):
                    return True
                
            self.left = TrueFilter()
            self.right = None
            self.combination = None
            return

        if s is not None:
            # Parse from a string to tokens, then combine tokens
            charbuffer = ""
            tokens = []
            had_backslash = False

            for c in s:
                if had_backslash:
                    charbuffer += c
                    had_backslash = False
                    continue

                if c == "\\":
                    had_backslash = True
                    continue

                if c == " ":
                    if charbuffer:
                        tokens.append(charbuffer)
                        charbuffer = ""
                    continue

                if c in "&|()":
                    if charbuffer:
                        tokens.append(charbuffer)
                        charbuffer = ""
                    tokens.append(c)
                    
                else:
                    charbuffer += c

            if had_backslash:
                raise ValueError(f"Invalid filter: {s}")
            if charbuffer:
                tokens.append(charbuffer)

        # Check for parens...
        first_paren_index = None
        last_paren_index = None
        for i in range(len(tokens)):
            if tokens[i] == "(" and first_paren_index is None:
                first_paren_index = i
            if tokens[i] == ")":
                last_paren_index = i

        if len([x for x in [first_paren_index, last_paren_index] if x is None]) == 1:
            raise ValueError(f"Invalid filter: {tokens}")

        # Recurse to get rid of parens if we have them
        if first_paren_index is not None and last_paren_index is not None:
            if first_paren_index + 1 >= last_paren_index:
                raise ValueError(f"Invalid filter: {tokens}")

            to_replace = Filter(tokens=tokens[first_paren_index + 1 : last_paren_index])
            tokens[first_paren_index : last_paren_index + 1] = [to_replace]

        def _make_atomic_filter(x):
            if isinstance(x, AtomicFilter) or isinstance(x, Filter):
                return x
            else:
                return AtomicFilter(x)

        # No more parens, just tokens, &, and |
        while True:
            try:
                and_index = tokens.index("&")
            except ValueError:
                break

            if and_index == 0 or and_index + 1 >= len(tokens):
                raise ValueError(f"Invalid filter: {tokens}")
            if (tokens[and_index - 1] == "&" or tokens[and_index - 1] == "|" or
                tokens[and_index + 1] == "&" or tokens[and_index + 1] == "|"):
                raise ValueError(f"Invalid filter: {tokens}")

            to_replace = Filter(left=_make_atomic_filter(tokens[and_index - 1]),
                                right=_make_atomic_filter(tokens[and_index + 1]),
                                combination="&")
            tokens[and_index - 1 : and_index + 2] = [to_replace]

        # tokens and |
        while True:
            try:
                or_index = tokens.index("|")
            except ValueError:
                break

            if or_index == 0 or or_index + 1 >= len(tokens):
                raise ValueError(f"Invalid filter: {tokens}")

            if tokens[or_index - 1] == "|" or tokens[or_index + 1] == "|":
                raise ValueError(f"Invalid filter: {tokens}")

            to_replace = Filter(left=_make_atomic_filter(tokens[or_index - 1]),
                                right=_make_atomic_filter(tokens[or_index + 1]),
                                combination="|")
            tokens[or_index - 1 : or_index + 2] = [to_replace]

        # tokens
        if len(tokens) != 1:
            raise ValueError(f"Invalid filter: {tokens}")

        self.left = _make_atomic_filter(tokens[0])
        self.right = None
        self.combination = None

    def run(self, old_kind, new_kind, type_) -> bool:
        l = self.left.run(old_kind, new_kind, type_)
        if self.right is not None:
            r = self.right.run(old_kind, new_kind, type_)

            if self.combination == "&":
                return l and r
            elif self.combination == "|":
                return l or r
            else:
                raise ValueException(f"Invalid filter:{self.__dict__}")
        else:
            return l

class Model(_ModelBase):
    """
    Does meta-analysis
    """
    def __init__(self, args):
        self.log = []
        self.filter = Filter(s=args.filter)
        
        self.new_kinds_to_types = {}
        self.new_types_to_kinds = {}
        self.old_kinds_to_types = {}
        self.old_types_to_kinds = {}
        self.new_kinds = set()
        self.old_kinds = set()
        self.new_types = set()
        self.old_types = set()

        new_path = self._find_path(args.new_path, args.analyzed_trait)
        old_path = self._find_path(args.old_path, args.analyzed_trait)

        print(f"Loading {new_path}...")
        self._load(new_path, self.new_kinds_to_types, self.new_types_to_kinds, self.new_kinds, self.new_types)
        print(f"Loading {old_path}...")
        self._load(old_path, self.old_kinds_to_types, self.old_types_to_kinds, self.old_kinds, self.old_types)
        print("")

        for meta_analysis in args.meta_analysis:
            self.log = []
            getattr(self, meta_analysis)()
            print("".join(self.log))

    def _run_all_filters(self, type_) -> bool:
        old_kind = self.old_types_to_kinds.get(type_)
        new_kind = self.new_types_to_kinds.get(type_)
        return self.filter.run(old_kind, new_kind, type_)

    def diff_kinds(self):
        with self._meta_analysis_pass("Difference of kinds"):
            for kind in self.new_kinds - self.old_kinds:
                self._log(f"  Added kind {kind}. ({len(self.new_kinds_to_types[kind])} values)")

            for kind in self.old_kinds - self.new_kinds:
                self._log(f"  Removed kind {kind}. ({len(self.old_kinds_to_types[kind])} values)")

    def diff_types(self):
        with self._meta_analysis_pass("Difference of types"):
            for type_ in self.new_types - self.old_types:
                if self._run_all_filters(type_):
                    self._log(f"  Added type {type_}: {self.new_types_to_kinds[type_]}")

            for type_ in self.old_types - self.new_types:
                if self._run_all_filters(type_):
                    self._log(f"  Removed type {type_}: {self.old_types_to_kinds[type_]}")

    def moved_types_summary(self):
        with self._meta_analysis_pass("Moved types summary"):
            move_summary = {}
            for type_ in self.new_types.intersection(self.old_types):
                if self._run_all_filters(type_): 
                    old_kind = self.old_types_to_kinds[type_]
                    new_kind = self.new_types_to_kinds[type_]
                    k = (old_kind, new_kind)
                    if k not in move_summary:
                        move_summary[k] = 0
                    move_summary[k] += 1

            for k, v in sorted(move_summary.items()):
                if k[0] == k[1]:
                    self._log(f"Stay at {k[0]}: {v} types")
                else:
                    self._log(f"Move from {k[0]}   ->  {k[1]}: {v} types")
            self._log("")


    def moved_types(self):
        with self._meta_analysis_pass("Moved types"):
            for type_ in self.new_types.intersection(self.old_types):
                if self._run_all_filters(type_):
                    if self.new_types_to_kinds[type_] != self.old_types_to_kinds[type_]:
                        self._log(f"Move from {self.old_types_to_kinds[type_]}   ->   {self.new_types_to_kinds[type_]}: {type_}")

    def all_types(self):
        with self._meta_analysis_pass("All types"):
            num_matches = 0
            for type_ in self.new_types:
                if self._run_all_filters(type_):
                    num_matches += 1
                    self._log(f"{self.new_types_to_kinds[type_]}: {type_}")

            self._log(f"{num_matches} types matched")

if __name__ == "__main__":
    default_analyzed_traits = ["CMakeParser", "Import", "PublicInheritance", "Typedef",
                               "Equatable", "Comparable", "Hashable", "CustomStringConvertible",
                               "FindSendableDependencies", "Sendable", "FindEnums",
                               "FindStaticTokens", "FindTfNoticeSubclasses",
                               "FindSchemas", "SdfValueTypeNamesMembers", "APINotes"]
    default_meta_analysis = ["diff_kinds", "diff_types", "moved_types_summary", "moved_types", "all_types"]
    
    parser = argparse.ArgumentParser(formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("new_path", help="The new path", type=pathlib.Path)
    parser.add_argument("old_path", help="The old path", type=pathlib.Path)
    parser.add_argument("--analyzed_traits", nargs="+", help=f"Analyzed trait (default: {default_analyzed_traits})", default=default_analyzed_traits)
    parser.add_argument("--meta_analysis", nargs="+", help=f"Meta analysis (default: {default_meta_analysis})", default=default_meta_analysis)
    parser.add_argument("--filter", help=f"Filters to suppress results. Can be combined with |, &, and ().\n{AtomicFilter.helpText()}", default="")

    args = parser.parse_args()

    for i in range(len(args.analyzed_traits)):
        args.analyzed_trait = args.analyzed_traits[i]
        model = Model(args)


