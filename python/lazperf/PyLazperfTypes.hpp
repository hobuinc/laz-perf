#pragma once

namespace pylazperf
{

// Stolen from PDAL

template<typename E>
constexpr typename std::underlying_type<E>::type toNative(E e)
{
    return static_cast<typename std::underlying_type<E>::type>(e);
}

enum class BaseType
{
    None = 0x000,
    Signed = 0x100,
    Unsigned = 0x200,
    Floating = 0x400
};

enum Type
{
    None = 0,
    Unsigned8 = toNative(BaseType::Unsigned) | 1,
    Signed8 = toNative(BaseType::Signed) | 1,
    Unsigned16 = toNative(BaseType::Unsigned) | 2,
    Signed16 = toNative(BaseType::Signed) | 2,
    Unsigned32 = toNative(BaseType::Unsigned) | 4,
    Signed32 = toNative(BaseType::Signed) | 4,
    Unsigned64 = toNative(BaseType::Unsigned) | 8,
    Signed64 = toNative(BaseType::Signed) | 8,
    Float = toNative(BaseType::Floating) | 4,
    Double = toNative(BaseType::Floating) | 8
};

inline std::string toName(BaseType b)
{
    switch (b)
    {
    case BaseType::Signed:
        return "signed";
    case BaseType::Unsigned:
        return "unsigned";
    case BaseType::Floating:
        return "floating";
    default:
        return "";
    }
}

inline BaseType base(Type t)
{
    return BaseType(toNative(t) & 0xFF00);
}

inline size_t size(Type t)
{
    return t & 0xFF;
}

inline std::string interpretationName(Type dimtype)
{
    switch (dimtype)
    {
    case Type::None:
        return "unknown";
    case Type::Signed8:
        return "int8_t";
    case Type::Signed16:
        return "int16_t";
    case Type::Signed32:
        return "int32_t";
    case Type::Signed64:
        return "int64_t";
    case Type::Unsigned8:
        return "uint8_t";
    case Type::Unsigned16:
        return "uint16_t";
    case Type::Unsigned32:
        return "uint32_t";
    case Type::Unsigned64:
        return "uint64_t";
    case Type::Float:
        return "float";
    case Type::Double:
        return "double";
    }
    return "unknown";
}


/// Get the type corresponding to a type name.
/// \param s  Name of type.
/// \return  Corresponding type enumeration value.
inline Type type(std::string s)
{
    if (s == "int8_t" || s == "int8")
       return Type::Signed8;
    if (s == "int16_t" || s == "int16")
       return Type::Signed16;
    if (s == "int32_t" || s == "int32")
       return Type::Signed32;
    if (s == "int64_t" || s == "int64")
       return Type::Signed64;
    if (s == "uint8_t" || s == "uint8")
        return Type::Unsigned8;
    if (s == "uint16_t" || s == "uint16")
        return Type::Unsigned16;
    if (s == "uint32_t" || s == "uint32")
        return Type::Unsigned32;
    if (s == "uint64_t" || s == "uint64")
        return Type::Unsigned64;
    if (s == "float")
        return Type::Float;
    if (s == "double")
        return Type::Double;
    return Type::None;
}


}
