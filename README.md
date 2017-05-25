# FishConfig
Header-Only, custom C++11 Configuration file loader and tree object system combined. 

Useful for any project with dynamic variables. 

Uses std and [boolinq](https://github.com/k06a/boolinq).

Initialization:
```c++
auto config = make_shared<Config>("config.txt");
config->readValuesFromFile();
auto global = make_shared<Configurable>("global", config);
```
Example usage:
```c++
class Square : public Configurable
{
  public:
	Square(const string &name, shared_ptr<Config> conf)
		:Configurable(name, conf)
	{
	  create(name);                 //Object has inserted itself into "conf" 
	  self()->create("dimensions")
  	    ->insert("width", ".1")   
    		->also("height", ".4");     //"also" useful for not rewriting entire statement for every tree element
    	  self()->insert("color", "yellow");
	}
};
...
auto square = new Square("Square", config);
square->me()["dimensions"]["width"].asInt = 10;       			//changes width property to 10
auto width = square->me()["dimensions"]["width"].as<int>();        	//"as" method available for other types
auto dimensions = square->me()["dimensions"].as<std::vector<float>>();  //only works if all children are floats
...
config->addVariable("version")->value = "1.0.1";      //all variables have a "value" string
                                                      //if directly set like this, the as*** variables are not available


```

