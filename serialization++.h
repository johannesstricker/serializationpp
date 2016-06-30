#pragma once
#include <type_traits>
#include <tuple>
#include <sstream>
#include <iostream>
#include <fstream>

/** DEPENDENCIES */
#include "json.hpp"

/** DEFINITIONS */
/** This is the name of the attribute, to hold the properties for serialization. */
#define PROPERTIES SERIALIZATION_PROPERTIES
/** Definitions for easier serialization. */
#define SERIALIZE(...) constexpr static auto PROPERTIES = std::make_tuple(__VA_ARGS__)
#define STORE(x,y) serialization::detail::makeProperty(x, y)

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

            template<typename T>
            void store(const char* name, const T& value);

            template<typename T>
            T retrieve(const char* name) const;
        };

        template<>
        void JsonArchive::store<int>(const char* name, const int& value)
        {
            storage[name] = value;
        }

        template<>
        void JsonArchive::store<std::string>(const char* name, const std::string& value)
        {
            storage[name] = value;
        }
        /**
         * When no specialization is known, we assume that this is a serializable object and try to
         * call serialize on it.
         */
        template<typename T>
        void JsonArchive::store(const char* name, const T& value)
        {
            storage[name] = serialize<JsonArchive>(value).getStorage();
        }

        template<>
        int JsonArchive::retrieve<int>(const char* name) const
        {
            return storage.at(name).ToInt();
        }
        template<>
        std::string JsonArchive::retrieve<std::string>(const char* name) const
        {
            return storage.at(name).ToString();
        }
        /**
         * When no specialization is known, we assume that this is a serializable object and try to
         * call deserialize on it.
         */
        template<typename T>
        T JsonArchive::retrieve(const char* name) const
        {
            T result;
            JsonArchive archive;
            archive.setStorage(storage.at(name));
            deserialize<JsonArchive>(archive, result);
            return result;
        }
    }
}



