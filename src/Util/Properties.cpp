// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#include <Util/Properties.h>
#include <Util/DisableWarnings.h>
#include <Util/StringUtil.h>
//#include <Util/FileUtil.h>
#include <Logging/Logger.h>
#include <Logging/LoggerUtil.h>
//#include <Util/ArgVector.h>
#include <Util/InitData.h>

using namespace std;
using namespace Util;
using namespace UtilInternal;

string
Util::Properties::GetProperty(const string& key)
{
    Util::Mutex::LockGuard sync(*this);

    map<string, PropertyValue>::iterator p = m_properties.find(key);
    if (p != m_properties.end())
    {
        p->second.used = true;
        return p->second.value;
    }
    else
    {
        return string();
    }
}

string
Util::Properties::GetPropertyWithDefault(const string& key, const string& value)
{
    Util::Mutex::LockGuard sync(*this);

    map<string, PropertyValue>::iterator p = m_properties.find(key);
    if (p != m_properties.end())
    {
        p->second.used = true;
        return p->second.value;
    }
    else
    {
        return value;
    }
}

Int
Util::Properties::GetPropertyAsInt(const string& key)
{
    return GetPropertyAsIntWithDefault(key, 0);
}

Int
Util::Properties::GetPropertyAsIntWithDefault(const string& key, Int value)
{
    Util::Mutex::LockGuard sync(*this);
    
    map<string, PropertyValue>::iterator p = m_properties.find(key);
    if (p != m_properties.end())
    {
        Int val = value;
        p->second.used = true;
        istringstream v(p->second.value);
        if (!(v >> value) || !v.eof())
        {
            Warning out(GetProcessLogger());
            out << "numeric property " << key << " set to non-numeric value, defaulting to " << val;
            return val;
        }
    }

    return value;
}

Util::StringSeq
Util::Properties::GetPropertyAsList(const string& key)
{
    return GetPropertyAsListWithDefault(key, StringSeq());
}

Util::StringSeq
Util::Properties::GetPropertyAsListWithDefault(const string& key, const StringSeq& value)
{
    Util::Mutex::LockGuard sync(*this);
    
    map<string, PropertyValue>::iterator p = m_properties.find(key);
    if (p != m_properties.end())
    {
        p->second.used = true;

        StringSeq result;
		if (!Util::String::SplitString(p->second.value, ", \t\r\n", result))
        {
            Warning out(GetProcessLogger());
            out << "mismatched quotes in property " << key << "'s value, returning default value";
        }
        if (result.size() == 0)
        {
            result = value;
        }
        return result;
    }
    else
    {
        return value;
    }
}


PropertyDict
Util::Properties::GetPropertiesForPrefix(const string& prefix)
{
    Util::Mutex::LockGuard sync(*this);

    PropertyDict result;
    for (map<string, PropertyValue>::iterator p = m_properties.begin(); p != m_properties.end(); ++p)
    {
        if (prefix.empty() || p->first.compare(0, prefix.size(), prefix) == 0)
        {
            p->second.used = true;
            result[p->first] = p->second.value;
        }
    }

    return result;
}

void
Util::Properties::SetProperty(const string& key, const string& value)
{
    //
    // Trim whitespace
    //
    string currentKey = Util::String::Trim(key);
    if (currentKey.empty())
    {
        throw InitializationException(__FILE__, __LINE__, "Attempt to set property with empty key");
    }

    Util::Mutex::LockGuard sync(*this);

    //
    // Set or clear the property.
    //
    if (!value.empty())
    {
        PropertyValue pv(value, false);
        map<string, PropertyValue>::const_iterator p = m_properties.find(currentKey);
        if (p != m_properties.end())
        {
            pv.used = p->second.used;
        }
        m_properties[currentKey] = pv;
    }
    else
    {
        m_properties.erase(currentKey);
    }
}

StringSeq
Util::Properties::GetCommandLineOptions()
{
    Util::Mutex::LockGuard sync(*this);

    StringSeq result;
    result.reserve(m_properties.size());
    for (map<string, PropertyValue>::const_iterator p = m_properties.begin(); p != m_properties.end(); ++p)
    {
        result.push_back("--" + p->first + "=" + p->second.value);
    }
    return result;
}

StringSeq
Util::Properties::ParseCommandLineOptions(const string& prefix, const StringSeq& options)
{
    string pfx = prefix;
    if (!pfx.empty() && pfx[pfx.size() - 1] != '.')
    {
        pfx += '.';
    }
    pfx = "--" + pfx;
    
    StringSeq result;
    for (StringSeq::size_type i = 0; i < options.size(); i++)
    {
        string opt = options[i];
       
        if (opt.find(pfx) == 0)
        {
            if (opt.find('=') == string::npos)
            {
                opt += "=1";
            }
            
            ParseLine(opt.substr(2), 0);
        }
        else
        {
            result.push_back(opt);
        }
    }
    return result;
}

void
Util::Properties::Load(const std::string& file)
{
//
// Metro style applications cannot access Windows registry.
//
#if defined (_WIN32) && !defined(OS_WINRT)
    if (file.find("HKLM\\") == 0)
    {
        HKEY iceKey;
        const wstring keyName = Util::StringToWstring(Util::NativeToUTF8(m_converter, file).substr(5)).c_str();
        LONG err;
        if ((err = RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyName.c_str(), 0, KEY_QUERY_VALUE, &iceKey)) != ERROR_SUCCESS)
        {
            InitializationException ex(__FILE__, __LINE__);
            ex.m_reason = "could not open Windows registry key `" + file + "':\n" + Util::ErrorToString(err);
            throw ex;
        }

        DWORD maxNameSize; // Size in characters not including terminating null character.
        DWORD maxDataSize; // Size in bytes
        DWORD numValues;
        try
        {
            err = RegQueryInfoKey(iceKey, NULL, NULL, NULL, NULL, NULL, NULL, &numValues, &maxNameSize, &maxDataSize, 
                                  NULL, NULL);
            if (err != ERROR_SUCCESS)
            {
                InitializationException ex(__FILE__, __LINE__);
                ex.m_reason = "could not open Windows registry key `" + file + "':\n";
                ex.m_reason += Util::ErrorToString(err);
                throw ex;
            }

            for (DWORD i = 0; i < numValues; ++i)
            {
                vector<wchar_t> nameBuf(maxNameSize + 1);
                vector<BYTE> dataBuf(maxDataSize);
                DWORD keyType;
                DWORD nameBufSize = static_cast<DWORD>(nameBuf.size());
                DWORD dataBufSize = static_cast<DWORD>(dataBuf.size());
                err = RegEnumValueW(iceKey, i, &nameBuf[0], &nameBufSize, NULL, &keyType, &dataBuf[0], &dataBufSize);
                if (err != ERROR_SUCCESS || nameBufSize == 0)
                {
                    ostringstream os;
                    os << "could not read Windows registry property name, key: `" + file + "', index: " << i << ":\n";
                    if (nameBufSize == 0)
                    {
                        os << "property name can't be the empty string";
                    }
                    else
                    {
                        os << Util::ErrorToString(err);
                    }
                    GetProcessLogger()->Warning(os.str());
                    continue;
                }
                string name = Util::WstringToString(wstring(reinterpret_cast<wchar_t*>(&nameBuf[0]), nameBufSize));
                name = Util::UTF8ToNative(m_converter, name);
                if (keyType != REG_SZ && keyType != REG_EXPAND_SZ)
                {
                    ostringstream os;
                    os << "unsupported type for Windows registry property `" + name + "' key: `" + file + "'";
                    GetProcessLogger()->Warning(os.str());
                    continue;
                }

                string value;
                wstring valueW = wstring(reinterpret_cast<wchar_t*>(&dataBuf[0]), (dataBufSize / sizeof(wchar_t)) - 1);
                if (keyType == REG_SZ)
                {
                    value = Util::WstringToString(valueW);
                }
                else // keyType == REG_EXPAND_SZ
                {
                    vector<wchar_t> expandedValue(1024);
                    DWORD sz = ExpandEnvironmentStringsW(valueW.c_str(), &expandedValue[0],
                                                         static_cast<DWORD>(expandedValue.size()));
                    if (sz >= expandedValue.size())
                    {
                        expandedValue.resize(sz + 1);
                        if (ExpandEnvironmentStringsW(valueW.c_str(), &expandedValue[0],
                                                     static_cast<DWORD>(expandedValue.size())) == 0)
                        {
                            ostringstream os;
                            os << "could not expand variable in property `" << name << "', key: `" + file + "':\n";
                            os << Util::LastErrorToString();
                            GetProcessLogger()->Warning(os.str());
                            continue;
                        }
                    }
                    value = Util::WstringToString(wstring(&expandedValue[0], sz -1));
                }
                value = Util::UTF8ToNative(m_converter, value);
                SetProperty(name, value);
            }
        }
        catch(...)
        {
            RegCloseKey(iceKey);
            throw;
        }
        RegCloseKey(iceKey);
    }
    else
#endif
    {
        UtilInternal::ifstream in(Util::NativeToUTF8(m_converter, file));
        if (!in)
        {
			throw FileException(__FILE__, __LINE__, UtilInternal::GetSystemErrno(), file);
        }

        string line;
        bool firstLine = true;
        while (getline(in, line))
        {
            //
            // Skip UTF8 BOM if present.
            //
            if (firstLine)
            {
                const unsigned char UTF8_BOM[3] = {0xEF, 0xBB, 0xBF}; 
                if (line.size() >= 3 &&
                   static_cast<const unsigned char>(line[0]) == UTF8_BOM[0] &&
                   static_cast<const unsigned char>(line[1]) == UTF8_BOM[1] && 
                   static_cast<const unsigned char>(line[2]) == UTF8_BOM[2])
                {
                    line = line.substr(3);
                }
                firstLine = false;
            }
            ParseLine(line, m_converter);
        }
    }
}

PropertiesPtr
Util::Properties::Clone()
{
    Util::Mutex::LockGuard sync(*this);
    return new Properties(this);
}

size_t 
Util::Properties::Size() const
{
	return m_properties.size();
}

set<string>
Util::Properties::GetUnusedProperties()
{
    Util::Mutex::LockGuard sync(*this);
    set<string> unusedProperties;
    for (map<string, PropertyValue>::const_iterator p = m_properties.begin(); p != m_properties.end(); ++p)
    {
        if (!p->second.used)
        {
            unusedProperties.insert(p->first);
        }
    }
    return unusedProperties;
}

Util::Properties::Properties(const Properties* p) :
    m_properties(p->m_properties),
    m_converter(p->m_converter)
{
}

Util::Properties::Properties(const StringConverterPtr& converter) :
    m_converter(converter)
{
}

Util::Properties::Properties(StringSeq& args, const PropertiesPtr& defaults, const StringConverterPtr& converter) :
    m_converter(converter)
{
    if (defaults != 0)
    {
        m_properties = static_cast<Properties*>(defaults.Get())->m_properties;
    }

    StringSeq::iterator q = args.begin();

    map<string, PropertyValue>::iterator p = m_properties.find("Util.ProgramName");
    if (p == m_properties.end())
    {
        if (q != args.end())
        {
            //
            // Use the first argument as the value for Util.ProgramName. Replace
            // any backslashes in this value with forward slashes, in case this
            // value is used by the event logger.
            //
            string name = *q;
            replace(name.begin(), name.end(), '\\', '/');

            PropertyValue pv(name, true);
            m_properties["Util.ProgramName"] = pv;
        }
    }
    else
    {
        p->second.used = true;
    }

    StringSeq tmp;

    bool loadConfigFiles = false;
    while (q != args.end())
    {
        string s = *q;
        if (s.find("--Util.Config") == 0)
        {
            if (s.find('=') == string::npos)
            {
                s += "=1";
            }
            ParseLine(s.substr(2), 0);
            loadConfigFiles = true;
        }
        else
        {
            tmp.push_back(s);
        }
        ++q;
    }
    args = tmp;

    if (!loadConfigFiles)
    {
        //
        // If Util.Config is not set, load from UTIL_CONFIG (if set)
        //
        loadConfigFiles = (m_properties.find("Util.Config") == m_properties.end());
    }

    if (loadConfigFiles)
    {
        LoadConfig();
    }
}

void
Util::Properties::ParseLine(const string& line, const StringConverterPtr& converter)
{
    string key;
    string value;
    
    enum ParseState { Key , Value };
    ParseState state = Key;

    string whitespace;
    string escapedspace;
    bool finished = false;
    for (string::size_type i = 0; i < line.size(); ++i)
    {
        char c = line[i];
        switch(state)
        {
          case Key:
          {
            switch(c)
            {
              case '\\':
                if (i < line.length() - 1)
                {
                    c = line[++i];
                    switch(c)
                    {
                      case '\\':
                      case '#':
                      case '=':
                        key += whitespace;
                        whitespace.clear();
                        key += c; 
                        break;

                      case ' ':
                        if (key.length() != 0)
                        {
                            whitespace += c;
                        }
                        break;

                      default:
                        key += whitespace;
                        whitespace.clear();
                        key += '\\';
                        key += c;
                        break;
                    }
                }
                else
                {
                    key += whitespace;
                    key += c;
                }
                break;

              case ' ':
              case '\t':
              case '\r':
              case '\n':
                  if (key.length() != 0)
                  {
                      whitespace += c;
                  }
                  break;

              case '=':
                  whitespace.clear();
                  state = Value;
                  break;

              case '#':
                  finished = true;
                  break;
              
              default:
                  key += whitespace;
                  whitespace.clear();
                  key += c;
                  break;
            }
            break;
          }

          case Value:
          {
            switch(c)
            {
              case '\\':
                if (i < line.length() - 1)
                {
                    c = line[++i];
                    switch(c)
                    {
                      case '\\':
                      case '#':
                      case '=':
                        value += value.length() == 0 ? escapedspace : whitespace;
                        whitespace.clear();
                        escapedspace.clear();
                        value += c; 
                        break;

                      case ' ':
                        whitespace += c;
                        escapedspace += c;
                        break;

                      default:
                        value += value.length() == 0 ? escapedspace : whitespace;
                        whitespace.clear();
                        escapedspace.clear();
                        value += '\\';
                        value += c;
                        break;
                    }
                }
                else
                {
					value += value.length() == 0 ? escapedspace : whitespace;
					value += c;
                }
                break;

              case ' ':
              case '\t':
              case '\r':
              case '\n':
                  if (value.length() != 0)
                  {
                      whitespace += c;
                  }
                  break;

              case '#':
                  value += escapedspace;
                  finished = true;
                  break;
              
              default:
                  value += value.length() == 0 ? escapedspace : whitespace;
                  whitespace.clear();
                  escapedspace.clear();
                  value += c;
                  break;
            }
            break;
          }
        }
        if (finished)
        {
            break;
        }
    }
    value += escapedspace;

    if ((state == Key && key.length() != 0) || (state == Value && key.length() == 0))
    {
        GetProcessLogger()->Warning("invalid config file entry: \"" + line + "\"");
        return;
    }
    else if (key.length() == 0)
    {
        return;
    }

    key = Util::UTF8ToNative(converter, key);
    value = Util::UTF8ToNative(converter, value);

    SetProperty(key, value);
}

void
Util::Properties::LoadConfig()
{
    string value = GetProperty("Util.Config");
#ifndef OS_WINRT
    //
    // WinRT cannot access environment variables
    if (value.empty() || value == "1")
    {
#   ifdef _WIN32
        vector<wchar_t> v(256);
        DWORD ret = GetEnvironmentVariableW(L"UTIL_CONFIG", &v[0], static_cast<DWORD>(v.size()));
        if (ret >= v.size())
        {
            v.resize(ret + 1);
            ret = GetEnvironmentVariableW(L"UTIL_CONFIG", &v[0], static_cast<DWORD>(v.size()));
        }
        if (ret > 0)
        {
            value = Util::UTF8ToNative(m_converter, Util::WstringToString(wstring(&v[0], ret)));
        }
        else
        {
            value = "";
        }
#   else
       const char* s = getenv("UTIL_CONFIG");
       if (s && *s != '\0')
       {
           value = s;
       }
#   endif
    }
#endif

    if (!value.empty())
    {
        const string delim = " \t\r\n";
        string::size_type beg = value.find_first_not_of(delim);
        while (beg != string::npos)
        {
            string::size_type end = value.find(",", beg);
            string file;
            if (end == string::npos)
            {
                file = value.substr(beg);
                beg = end;
            }
            else
            {
                file = value.substr(beg, end - beg);
                beg = value.find_first_not_of("," + delim, end);
            }
            Load(file);
        }
    }

    PropertyValue pv(value, true);
    m_properties["Util.Config"] = pv;
}
