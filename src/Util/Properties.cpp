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
#include <Logging/Logger.h>
#include <Logging/LoggerUtil.h>

using namespace std;
using namespace Threading;


string
Threading::Properties::GetProperty(const string& key)
{
    Threading::Mutex::LockGuard sync(*this);

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
Threading::Properties::GetPropertyWithDefault(const string& key, const string& value)
{
    Threading::Mutex::LockGuard sync(*this);

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
Threading::Properties::GetPropertyAsInt(const string& key)
{
    return GetPropertyAsIntWithDefault(key, 0);
}

Int
Threading::Properties::GetPropertyAsIntWithDefault(const string& key, Int value)
{
    Threading::Mutex::LockGuard sync(*this);
    
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

Threading::StringSeq
Threading::Properties::GetPropertyAsList(const string& key)
{
    return GetPropertyAsListWithDefault(key, StringSeq());
}

Threading::StringSeq
Threading::Properties::GetPropertyAsListWithDefault(const string& key, const StringSeq& value)
{
    Threading::Mutex::LockGuard sync(*this);
    
    map<string, PropertyValue>::iterator p = m_properties.find(key);
    if (p != m_properties.end())
    {
        p->second.used = true;

        StringSeq result;
        if (!Threading::SplitString(p->second.value, ", \t\r\n", result))
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
Threading::Properties::GetPropertiesForPrefix(const string& prefix)
{
    Threading::Mutex::LockGuard sync(*this);

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
Threading::Properties::SetProperty(const string& key, const string& value)
{
    //
    // Trim whitespace
    //
    string currentKey = Threading::Trim(key);
    if (currentKey.empty())
    {
        throw InitializationException(__FILE__, __LINE__, "Attempt to set property with empty key");
    }

    Threading::Mutex::LockGuard sync(*this);

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
Threading::Properties::GetCommandLineOptions()
{
    Threading::Mutex::LockGuard sync(*this);

    StringSeq result;
    result.reserve(m_properties.size());
    for (map<string, PropertyValue>::const_iterator p = m_properties.begin(); p != m_properties.end(); ++p)
    {
        result.push_back("--" + p->first + "=" + p->second.value);
    }
    return result;
}

StringSeq
Threading::Properties::ParseCommandLineOptions(const string& prefix, const StringSeq& options)
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
Threading::Properties::Load(const std::string& file)
{
    Threading::ifstream in(Threading::NativeToUTF8(m_converter, file));
    if (!in)
    {
        throw FileException(__FILE__, __LINE__, Threading::GetSystemErrno(), file);
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

PropertiesPtr
Threading::Properties::Clone()
{
    Threading::Mutex::LockGuard sync(*this);
    return new Properties(this);
}

size_t 
Threading::Properties::Size() const
{
    return m_properties.size();
}

set<string>
Threading::Properties::GetUnusedProperties()
{
    Threading::Mutex::LockGuard sync(*this);
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

Threading::Properties::Properties(const Properties* p) :
    m_properties(p->m_properties),
    m_converter(p->m_converter)
{
}

Threading::Properties::Properties(const StringConverterPtr& converter) :
    m_converter(converter)
{
}

void
Threading::Properties::ParseLine(const string& line, const StringConverterPtr& converter)
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

    key = Threading::UTF8ToNative(converter, key);
    value = Threading::UTF8ToNative(converter, value);

    SetProperty(key, value);
}

void
Threading::Properties::LoadConfig()
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
            value = Threading::UTF8ToNative(m_converter, Threading::WstringToString(wstring(&v[0], ret)));
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

PropertiesPtr
Threading::CreateProperties(const StringConverterPtr& converter)
{
    return new Properties(converter);
}
