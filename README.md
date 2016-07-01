# serialization++
A simple C++ serialization library. In the making.

### Dependencies
The JSON-serialization depends on [SimpleJSON](https://github.com/nbsdx/SimpleJSON). Make sure to have the json.hpp in your include directory.

### License
Use it however you want.

### Examples
```cpp
    class Human
    {
    protected:
        std::string name;
        int age;
    public:
        Human(const std::string& aName = "", int aAge = 0)
        : name(aName), age(aAge) {}

        // name all the properties you want to serialize
        // IMPORTANT: all included attributes have to be declared before the macro
        SERIALIZE(
            STORE(&Human::name, "name"),
            STORE(&Human::age, "age")
        );
    };

    class Parent : public Human
    {
    protected:
        Human child;
    public:
        Parent(const std::string& aName = "", int aAge = 0, const Human& aChild = Human())
        : Human(aName, aAge), child(aChild) {}

        // when you want to serialize additional properties on the subclass
        // you have to override the SERIALIZE macro
        SERIALIZE(
            STORE(&Parent::name, "name"),
            STORE(&Parent::age, "age"),
            STORE(&Parent::child, "child")
        );
    };
}
```
#### serialize
```cpp
    Human child("Mark Zuckerberg", 32);
    Parent parent("Steve Jobs", 56, child);

    archive = serialization::serialize<serialization::archive::JsonArchive>(parent);
    archive.saveToFile("parent.json");
```
#### deserialize
```cpp
    Human markZuckerberg;
    Parent steveJobs;

    JsonArchive archive;
    archive.loadFromFile("parent.json");
    serialization::deserialize<serialization::archive::JsonArchive>(archive, steveJobs);
```