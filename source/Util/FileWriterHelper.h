//
//  FileWriterHelper.h
//  ast-answerer
//
//  Created by Maddy Adams on 4/17/24.
//

#ifndef FileWriterHelper_h
#define FileWriterHelper_h

#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>

// Helper class for writing files. Automatically generates prelude and epilogues based on the file type. 
class FileWriterHelper {
public:
    FileWriterHelper(const std::string& fileName);
    FileWriterHelper(const std::filesystem::path& directory,
                     const std::string& fileName);
    
    ~FileWriterHelper();
    
    void setFileName(const std::string& fileName);
    void setDirectory(const std::filesystem::path& directory);
    
    void openHeaderFile(const std::string& headerGuardToken = "");
    void openCppFile();
    void openMmFile();
    void openSwiftFile();
    void openModulemapFile();
    void openAPINotesFile();
    void openDocCFile();
    
    
    void addLine(const std::string& line);
    void addLines(const std::vector<std::string>& lines);
    void closeFile();
    
    bool hasOpenFile() const;
    std::string getOpenFileSuffix() const;
    
    bool getWritesPrologue() const;
    void setWritesPrologue(bool newValue);
    
private:
    void _openFile(const std::string& suffix);
    std::string _fileNameWithSuffix() const;
    
    std::vector<std::string> _getPrologue() const;
    std::vector<std::string> _getEpilogue() const;
    
    std::string _fileName;
    std::string _openFileSuffix;
    std::filesystem::path _directory;
    std::string _headerFileName;
    std::string _headerGuardToken;
    std::ofstream _stream;
    bool _writesPrologue;
    
    std::vector<std::string> __lines;
};

#endif /* FileWriterHelper_h */
