#pragma once
#include <type_traits>
#include <tuple>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>

/** DEPENDENCIES */
#include "json.hpp"

/** DEFINITIONS */
/** This is the name of the attribute, to hold the properties for serialization. */
#define PROPERTIES SERIALIZATION_PROPERTIES
/** Returns true if T has the PROPERTIES attribute. */
#define IF_SERIALIZABLE(T, RETURN_TYPE) std::enable_if_t<serialization::detail::has_properties<T>::value, RETURN_TYPE>
#define IF_NOT_SERIALIZABLE(T, RETURN_TYPE) std::enable_if_t<!serialization::detail::has_properties<T>::value, RETURN_TYPE>
/** Definitions for easier serialization. */
#define SERIALIZE(...) constexpr static auto PROPERTIES = std::make_tuple(__VA_ARGS__)
#define STORE(x,y) serialization::detail::makeProperty(x, y)


/** http://ideone.com/yd0dhs */
namespace serialization
{
    /**
     * The detail-namespace contains everything that should be hidden to the outside world.
     */
    namespace detail
    {
        template<typename Class, typename T>
        struct Property
        {
            constexpr Property(T Class::*aMember, const char* aName)
            : member{aMember}, name{aName}
            {
                // empty
            }
            using Type = T;
            T Class::*member;
            const char* name;
        };

        template<typename Class, typename T>
        constexpr auto makeProperty(T Class::*member, const char* name)
        {
            return serialization::detail::Property<Class, T>{member, name};
        }

        /**
         * Used to check if a Type has the properties member, and thus is serializable.
         */
        template<typename T, typename = void>
        struct has_properties : std::false_type { };
        template<typename T>
        struct has_properties<T, decltype((T::PROPERTIES), void())> : std::true_type { };
        /**
         * Used to check if a Type is a std::pair.
         */
        template<typename T, typename = void>
        struct is_pair : std::false_type { };
        template<typename T>
        struct is_pair<T, decltype(T::first, T::second, void())> : std::true_type { };
        /**
         * Used to check if a type is iterable, eg. has an iterator.
         */
        template<typename C>
        struct is_iterable
        {
          typedef long false_type;
          typedef char true_type;

          template<class T> static false_type check(...);
          template<class T> static true_type  check(int, typename T::const_iterator = C().end());

          enum { value = sizeof(check<C>(0)) == sizeof(true_type) };
        };

        /**
         * DESERIALIZATION HELPER FUNCTIONS *
         */
        template<std::size_t iteration, typename T, typename IArchive>
        void doSetData(T&& object, const IArchive& archive)
        {
            constexpr auto property = std::get<iteration>(std::decay_t<T>::PROPERTIES);
            using Type = typename decltype(property)::Type;
            object.*(property.member) = archive.template retrieve<Type>(property.name);
        }

        /**
         * If iteration == 0, stop iterating.
         */
        template<std::size_t iteration, typename T, typename IArchive>
        std::enable_if_t<(iteration <= 0)>
        setData(T&& object, const IArchive& archive)
        {
            serialization::detail::doSetData<iteration, T, IArchive>(object, archive);
        }

        /**
         * If iteration > 0, continue iterating.
         */
        template<std::size_t iteration, typename T, typename IArchive>
        std::enable_if_t<(iteration > 0)>
        setData(T&& object, const IArchive& archive)
        {
            serialization::detail::doSetData<iteration, T, IArchive>(object, archive);
            serialization::detail::setData<(iteration - 1), T, IArchive>(object, archive);
        }

        /**
         * SERIALIZATION HELPER FUNCTIONS *
         */
        template<std::size_t iteration, typename T, typename IArchive>
        void doGetData(T&& object, IArchive& archive)
        {
            constexpr auto property = std::get<iteration>(std::decay_t<T>::PROPERTIES);
            using Type = typename decltype(property)::Type;
            archive.template store<Type>(property.name, object.*(property.member));
        }

        /**
         * If iteration == 0, stop iterating.
         */
        template<std::size_t iteration, typename T, typename IArchive>
        std::enable_if_t<(iteration <= 0)>
        getData(T&& object, IArchive& archive)
        {
            serialization::detail::doGetData<iteration, T, IArchive>(object, archive);
        }

        /**
         * If iteration > 0, continue iterating.
         */
        template<std::size_t iteration, typename T, typename IArchive>
        std::enable_if_t<(iteration > 0)>
        getData(T&& object, IArchive& archive)
        {
            serialization::detail::doGetData<iteration, T, IArchive>(object, archive);
            serialization::detail::getData<(iteration - 1), T, IArchive>(object, archive);
        }
    }

    /**
     * Takes an IArchive and writes the contained properties to the object. Object has to have
     * the SERIALIZATION-macro.
     */
    template<typename IArchive, typename T>
    bool deserialize(const IArchive& archive, T &obj)
    {
        detail::setData<(std::tuple_size<decltype(T::PROPERTIES)>::value - 1)>(obj, archive);
        return true;
    };

    /**
     * Takes an object with the SERIALIZE-macro and stores it's properties in an IArchive.
     */
    template<typename IArchive, typename T>
    IArchive serialize(const T &obj)
    {
        IArchive archive;
        detail::getData<(std::tuple_size<decltype(T::PROPERTIES)>::value - 1)>(obj, archive);
        return archive;
    }


    /**
     * The archive-namespace contains different Archive implementations, to store object in to different formats.
     */
    namespace archive
    {
        class IArchive
        {
        public:
            virtual bool saveToFile(const std::string& filepath) = 0;
            virtual bool loadFromFile(const std::string& filepath) = 0;
        };

        class JsonArchive : public IArchive
        {
        private:
            json::JSON storage;
        public:
            json::JSON getStorage()
            {
                return storage;
            }
            void setStorage(const json::JSON aStorage)
            {
                storage = aStorage;
            }

            bool saveToFile(const std::string& filepath) override
            {
                std::ofstream file(filepath);
                file << storage;
                file.close();
                return true;
            }
            bool loadFromFile(const std::string& filepath) override
            {
                std::ifstream file(filepath);
                std::string fileContents { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
                storage = json::JSON::Load(fileContents);
                return true;
            }


            // template<typename T>
            // IS_STL_CONTAINER(T, void) store(const char* name, const T& value);

            template<typename T>
            IF_SERIALIZABLE(T, void) store(const char* name, const T& value);

            template<typename T>
            IF_NOT_SERIALIZABLE(T, void) store(const char* name, const T& value);


            // template<typename T>
            // IS_STL_CONTAINER(T, T) retrieve(const char* name) const;

            template<typename T>
            IF_SERIALIZABLE(T, T) retrieve(const char* name) const;

            template<typename T>
            IF_NOT_SERIALIZABLE(T, T) retrieve(const char* name) const;
        };

        // template<typename T>
        // IS_STL_CONTAINER(T, void) JsonArchive::store(const char* name, const T& value)
        // {
        //     JsonArchive archive;
        //     int i = 0;
        //     for(auto iterator = value.begin(); iterator != value.end(); iterator++)
        //     {
        //         archive.store(i++, *iterator);
        //     }
        //     storage[name] = archive;
        // }

        template<typename T>
        IF_SERIALIZABLE(T, void) JsonArchive::store(const char* name, const T& value)
        {
            storage[name] = serialize<JsonArchive>(value).getStorage();
        }

        template<typename T>
        IF_NOT_SERIALIZABLE(T, void) JsonArchive::store(const char* name, const T& value)
        {
            storage[name] = value;
        }



        // template<typename T>
        // IS_STL_CONTAINER(T, T) retrieve(const char* name) const
        // {
        //     T result;
        //     JsonArchive archive;
        //     archive.setStorage(storage.at(name));
        //     deserialize<JsonArchive>(archive, result);
        //     return result;
        // }

        template<>
        IF_NOT_SERIALIZABLE(int, int) JsonArchive::retrieve<int>(const char* name) const
        {
            return storage.at(name).ToInt();
        }

        template<>
        IF_NOT_SERIALIZABLE(std::string, std::string) JsonArchive::retrieve<std::string>(const char* name) const
        {
            return storage.at(name).ToString();
        }

        // template<typename T>
        // IF_IS_CONTAINER(T, T) JsonArchive::retrieve(const char* name) const
        // {
        //     std::vector<V> result;
        //     auto jsonValue = storage.at(name);
        //     for(int i=0; i<jsonValue.length(); i++)
        //     {
        //         JsonArchive archive;
        //         archive.setStorage(jsonValue.at(i));
        //         V value;
        //         deserialize<JsonArchive>(archive, value);
        //         result.push_back(value);
        //     }
        //     return result;
        // }

        template<typename T>
        IF_SERIALIZABLE(T, T) JsonArchive::retrieve(const char* name) const
        {
            T result;
            JsonArchive archive;
            archive.setStorage(storage.at(name));
            deserialize<JsonArchive>(archive, result);
            return result;
        }
    }
}



