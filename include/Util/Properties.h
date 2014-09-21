// **********************************************************************
//
// Copyright (c) 2010-2014 Bernard Luo. All rights reserved.
//
// <Email: luo (dot) xiaowei (at) hotmail (dot) com>
//
// **********************************************************************

#ifndef UTIL_PROPERTIES_H
#define UTIL_PROPERTIES_H

#include <Concurrency/Mutex.h>
//#include <Util/Properties.h>
#include <Util/StringConverter.h>

#include <set>
#include <map>

namespace Util
{

/**
 *
 * A simple collection of properties, represented as a dictionary of
 * key/value pairs. Both key and value are strings.
 *
 * @see Properties#getPropertiesForPrefix
 *
 **/
typedef std::map<std::string, std::string> PropertyDict;

class Properties;
typedef SharedPtr<Properties> PropertiesPtr;

class UTIL_API Properties : public Util::Mutex, public Shared
{
public:

	virtual std::string GetProperty(const std::string& key);
	virtual std::string GetPropertyWithDefault(const std::string& key, const std::string& value);
	virtual Util::Int GetPropertyAsInt(const std::string& key);
	virtual Util::Int GetPropertyAsIntWithDefault(const std::string& key, Util::Int value);
	virtual Util::StringSeq GetPropertyAsList(const std::string& key);
	virtual Util::StringSeq GetPropertyAsListWithDefault(const std::string& key, const Util::StringSeq& value);

	virtual PropertyDict GetPropertiesForPrefix(const std::string& prefix);
	virtual void SetProperty(const std::string& key, const std::string& value);
	virtual StringSeq GetCommandLineOptions();
	virtual StringSeq ParseCommandLineOptions(const std::string& prefix, const StringSeq& options);
	virtual void Load(const std::string& file);
	virtual PropertiesPtr Clone();
	size_t Size() const;

	std::set<std::string> GetUnusedProperties();
	
private:
	Properties(const StringConverterPtr& converter);
	Properties(StringSeq& args, const PropertiesPtr& defaults, const StringConverterPtr& converter);
	Properties(const Properties* properties);

	friend UTIL_API PropertiesPtr CreateProperties(const StringConverterPtr&);
	friend UTIL_API PropertiesPtr CreateProperties(StringSeq&, const PropertiesPtr&, const StringConverterPtr&);
	friend UTIL_API PropertiesPtr CreateProperties(int&, char*[], const PropertiesPtr&, const StringConverterPtr&);

	void ParseLine(const std::string& line, const StringConverterPtr& converter);

	void LoadConfig();

	struct PropertyValue
	{
		PropertyValue() : used(false)
		{
		}

		PropertyValue(const std::string& v, bool u) : value(v), used(u)
		{
		}

		std::string value;
		bool used;
	};
	std::map<std::string, PropertyValue> m_properties;
	const StringConverterPtr m_converter;
};

}

#endif
