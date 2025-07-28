//
//  TfNoticeProtocolCodeGen.cpp
//  ast-answerer
//
//  Created by Maddy Adams on 1/6/25.
//

#include "CodeGen/TfNoticeProtocolCodeGen.h"
#include "AnalysisPass/FindTfNoticeSubclassesAnalysisPass.h"
#include <iostream>

TfNoticeProtocolCodeGen::TfNoticeProtocolCodeGen(const CodeGenRunner* codeGenRunner) : CodeGenBase<TfNoticeProtocolCodeGen>(codeGenRunner) {}

std::string TfNoticeProtocolCodeGen::fileNamePrefix() const {
    return "TfNoticeProtocolConformances";
}

TfNoticeProtocolCodeGen::Data TfNoticeProtocolCodeGen::preprocess() {
    const FindTfNoticeSubclassesAnalysisPass* findTfNoticeSubclassesAnalysisPass = getFindTfNoticeSubclassesAnalysisPass();
    
    TfNoticeProtocolCodeGen::Data result;
    for (const auto& it : findTfNoticeSubclassesAnalysisPass->getData()) {
        result.push_back(clang::dyn_cast<clang::TagDecl>(it.first));
    }
    return result;
}

void TfNoticeProtocolCodeGen::writeSwiftFile(const TfNoticeProtocolCodeGen::Data& data) {
    writeLines({
        "fileprivate func _convertCallback(_ callback: @escaping (borrowing pxr.TfNotice.NoticeCaster) -> ()) -> (__SwiftUsd.TfNotice.PointerHolder) -> () {",
        "    { (p: __SwiftUsd.TfNotice.PointerHolder) in",
        "        callback(.init(p.notice))",
        "    }",
        "}",
        "fileprivate func _convertCallback(_ callback: @escaping (pxr.TfAnyWeakPtr, borrowing pxr.TfNotice.NoticeCaster) -> ()) -> (pxr.TfAnyWeakPtr, __SwiftUsd.TfNotice.PointerHolder) -> () {",
        "    { (s: pxr.TfAnyWeakPtr, p: __SwiftUsd.TfNotice.PointerHolder) in",
        "        callback(s, .init(p.notice))",
        "    }",
        "}",
    });
    
    for (const auto& x : data) {
        if (!hasTypeName<SwiftNameInSwift>(x)) { continue; }
        auto printer = typeNamePrinter(x);
        std::string swiftName = getTypeName<SwiftNameInSwift>(printer);
        
        writeLines({
            "extension " + swiftName + ": SwiftUsd.TfNoticeProtocol {",
            "    public static func _Register(_ callback: @escaping (borrowing pxr.TfNotice.NoticeCaster) -> ()) -> pxr.TfNotice.SwiftKey {",
            "        var key = pxr.TfNotice.SwiftKey()",
            "        __SwiftUsd.TfNotice.Register(_convertCallback(callback), &key, Notice: Self.self)",
            "        return key",
            "    }",
            "    public static func _Register(_ sender: pxr.TfAnyWeakPtr, _ callback: @escaping (borrowing pxr.TfNotice.NoticeCaster) -> ()) -> pxr.TfNotice.SwiftKey {",
            "        var key = pxr.TfNotice.SwiftKey()",
            "        __SwiftUsd.TfNotice.Register(_convertCallback(callback), sender, &key, Notice: Self.self)",
            "        return key",
            "    }",
            "    public static func _Register(_ sender: pxr.TfAnyWeakPtr, _ callback: @escaping (pxr.TfAnyWeakPtr, borrowing pxr.TfNotice.NoticeCaster) -> ()) -> pxr.TfNotice.SwiftKey {",
            "        var key = pxr.TfNotice.SwiftKey()",
            "        __SwiftUsd.TfNotice.Register(_convertCallback(callback), sender, &key, Notice: Self.self)",
            "        return key",
            "    }",
            "",
            "    public static func _dynamic_cast(_ p: UnsafePointer<pxr.TfNotice>) -> UnsafePointer<Self>? {",
            "        var result: UnsafePointer<Self>?",
            "        __SwiftUsd.TfNotice.dyn_cast(p, &result)",
            "        return result",
            "    }",
            "}",
        });
    }
}
