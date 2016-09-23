#ifndef CONFIG_READER_H
#define CONFIG_READER_H
#include <string>
#include <map>
#include <fstream>
#include <memory>
#include "boolinq.h"
#include <sstream>
using namespace std;

class Config;
class Variable;

class Configurable
{
public:
	Configurable(const string &name, shared_ptr<Config> config);

	virtual shared_ptr<Variable> insert(const string &var, const string &val);
	virtual shared_ptr<Variable> create(const string &var);
	shared_ptr<Variable> select(const string &var);
	shared_ptr<Variable> self();
	Variable &me();
	virtual shared_ptr<Variable> also(const string &var, const string &val);
	Variable &operator[](const string &var);
	template <class T> const T as();
	string name;

	shared_ptr<Variable> last;
	//Pointer because this value will change outside of this class
	shared_ptr<Config> config;
	shared_ptr<Config> referencedFrom;
};



class Variable : public Configurable, public std::enable_shared_from_this<Variable>
{

public:
	Variable(): value(""),
		Configurable("", make_shared<Config>())
	{
		cached = false;
	}

	Variable(string name, string defaultValue) : value(defaultValue),
		Configurable(name, make_shared<Config>())
	{
		cached = false;
	}

	bool isInt(const std::string& s)
	{
		std::string::const_iterator it = s.begin();
		while (it != s.end() && isdigit(*it))++it;
		return !s.empty() && it == s.end();
	}

	bool isFloat(std::string myString)
	{
		std::istringstream iss(myString);
		float f;
		iss >> std::noskipws >> f;
		return iss.eof() && !iss.fail();
	}

	virtual bool invalid()
	{
		return false;
	}

	void operator=(string value)
	{
		this->value = value;
	}

	bool cached;
	string value;
	union
	{
		float asFloat;
		int asInt;
		bool asBool;
	};
};

class NullVariable : public Variable
{

public:
	NullVariable() :
		Variable("", "")
	{
	}

	shared_ptr<Variable> insert(const string & var, const string & val) override
	{
		return this->shared_from_this();
	}

	shared_ptr<Variable> create(const string &var) override
	{
		return this->shared_from_this();
	}

	shared_ptr<Variable> also(const string &var, const string &val) override
	{
		return this->shared_from_this();
	}

	bool invalid() override
	{
		return true;
	}
	
};

class Config : public std::enable_shared_from_this<Config>
{
public:
	Config()
	{
		configFile = "config.txt";
	}
	
	void readValuesFromFile()
	{
		string line;
		ifstream file(configFile);
		if (file.is_open())
		{
			while (getline(file, line))
			{
				bool intoKey = true;
				string str;
				auto currentVar = make_shared<Variable>();
				auto parentVar = currentVar;
				for (auto c : line)
				{
					if (c == ':')
					{
						if (currentVar->name == "")
						{
							parentVar = currentVar;
							currentVar->name = str;
						}
						else
							currentVar = currentVar->create(str);
						str.clear();
						str.clear();
					}
					else
					{
						str += c;
					}

				}
				currentVar->value = str;
				variables[parentVar->name] = parentVar;
				variables[parentVar->name]->referencedFrom = this->shared_from_this();

			}
			file.close();
		}
	}

	string viewAll(Config *above = nullptr, int iterations = 0)
	{
		string tree;
		for (auto &var : above == nullptr? variables : above->variables)
		{
			if (var.first == "" && var.second->value == "")
				continue;
			string tabs;
			for (int i = 0; i < iterations; i++)
				tabs += "\t";
			tree += tabs + var.first + ":" + var.second->value + "\n";
			tree += above->viewAll(var.second->config.get(), iterations+1);
		}
		return tree;
	}

	template <class T>
	const T valueOf(const string& key)
	{
		if (variables.find(key) == variables.end())
			return T();
		using namespace boolinq;
		return T(
			from(variables[key]->config->getVariables()).
			select([](auto x) {return x.asFloat; }).
			toVector());
	}

	shared_ptr<Variable> getVariable(string var)
	{
		if (variables.find(var) != variables.end())
			return variables[var];
		return make_shared<NullVariable>();
	}

	shared_ptr<Variable> addVariable(string var)
	{
		if (variables.find(var) == variables.end())
		{
			variables[var] = make_shared<Variable>();
			variables[var]->name = var;
			return variables[var];
		}
		return  make_shared<NullVariable>();
	}

	const vector<string> &getVariableNames()
	{
		using namespace boolinq;
		return from(variables)
			.select([](auto x) {return get<0>(x); })
			.toVector();
	}

	const vector<shared_ptr<Variable>> &getVariables()
	{
		using namespace boolinq;
		return from(variables)
			.select([](auto x) {return get<1>(x); })
			.toVector();
	}

	map<string, shared_ptr<Variable>> variables;
	string configFile;
};


template <>
const int Config::valueOf<int>(const string& key)
{
	if (variables.find(key) == variables.end())
		return int();
	return variables[key]->asInt;
}

template <>
const bool Config::valueOf<bool>(const string& key)
{
	if (variables.find(key) == variables.end())
		return bool();
	return variables[key]->asBool;
}

template <>
const string Config::valueOf<string>(const string& key)
{
	if (variables.find(key) == variables.end())
		return string();
	return variables[key]->value;
}

template <>
const float Config::valueOf<float>(const string& key)
{
	if (variables.find(key) == variables.end())
		return float();
	return variables[key]->asFloat;

}

Configurable::Configurable(const string& name, shared_ptr<Config> config): name(name), config(config)
{
}

shared_ptr<Variable> Configurable::insert(const string & var, const string & val)
{

	last = config->addVariable(var);
	last->value = val;
	if (last->isFloat(val))
		last->asFloat = stof(val);
	if(last->isInt(val))
		last->asInt = stoi(val);
	if (val == "TRUE" || val == "FALSE" || val == "true" || val == "false")
		last->asBool = (val == "TRUE" || val == "true") ? true : false;
	last->referencedFrom = config->shared_from_this();
	return last->shared_from_this();
}

shared_ptr<Variable> Configurable::create(const string &var)
{
	last = config->addVariable(var);
	last->referencedFrom = config->shared_from_this();
	return last->shared_from_this();
}

shared_ptr<Variable> Configurable::self()
{
	last = config->getVariable(name);
	return last->shared_from_this();
}

inline Variable & Configurable::me()
{
	return *self().get();
}

shared_ptr<Variable> Configurable::select(const string &var)
{
	last = config->getVariable(var);
	return last->shared_from_this();
}

inline Variable &Configurable::operator[](const string & var)
{
	return *select(var).get();
}

shared_ptr<Variable> Configurable::also(const string & var, const string & val)
{
	auto newVariable = referencedFrom->addVariable(var);
	newVariable->value = val;
	if (newVariable->isFloat(val))
		newVariable->asFloat = stof(val);
	else if (newVariable->isInt(val))
		newVariable->asInt = stoi(val);
	else if (val == "TRUE" || val == "FALSE" || val == "true" || val == "false")
		newVariable->asBool = (val == "TRUE" || val == "true") ? true : false;
	newVariable->referencedFrom = referencedFrom;
	return newVariable->shared_from_this();
}

template <class T>
const T Configurable::as()
{
	if (referencedFrom != nullptr)
		return referencedFrom->valueOf<T>(name);
	else
		return config->valueOf<T>(name);
}


#endif