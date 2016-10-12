#pragma once

#include "decode/Config.h"
#include "decode/Rc.h"
#include "decode/parser/FileInfo.h"

#include <bmcl/StringView.h>

namespace decode {
namespace parser {

class ModuleInfo : public RefCountable {
public:
    ModuleInfo(bmcl::StringView name, const Rc<FileInfo>& fileInfo);

    bmcl::StringView moduleName() const;
    const Rc<FileInfo> fileInfo() const;
    const std::string& fileName() const;
    const std::string& contents() const;

private:
    friend class Parser;
    ModuleInfo() = default;

    bmcl::StringView _moduleName;
    Rc<FileInfo> _fileInfo;
};

inline ModuleInfo::ModuleInfo(bmcl::StringView name, const Rc<FileInfo>& fileInfo)
    : _moduleName(name)
    , _fileInfo(fileInfo)
{
}

inline bmcl::StringView ModuleInfo::moduleName() const
{
    return _moduleName;
}

inline const Rc<FileInfo> ModuleInfo::fileInfo() const
{
    return _fileInfo;
}

inline const std::string& ModuleInfo::contents() const
{
    return _fileInfo->contents();
}

inline const std::string& ModuleInfo::fileName() const
{
    return _fileInfo->fileName();
}
}
}

