//
// Created by maxwe on 2021-12-07.
//

#ifndef JEMSYS_AGATE2_INTERNAL_FLAGS_HPP
#define JEMSYS_AGATE2_INTERNAL_FLAGS_HPP

#define AGT_BITFLAG_ENUM(EnumType, UnderlyingType) \
  enum class EnumType : UnderlyingType;               \
  JEM_forceinline constexpr EnumType operator~(EnumType a) noexcept { \
    return static_cast<EnumType>(~static_cast<UnderlyingType>(a)); \
  }                                                  \
  JEM_forceinline constexpr EnumType operator&(EnumType a, EnumType b) noexcept { \
    return static_cast<EnumType>(static_cast<UnderlyingType>(a) & static_cast<UnderlyingType>(b)); \
  }                                                  \
  JEM_forceinline constexpr EnumType operator|(EnumType a, EnumType b) noexcept { \
    return static_cast<EnumType>(static_cast<UnderlyingType>(a) | static_cast<UnderlyingType>(b)); \
  }                                                  \
  JEM_forceinline constexpr EnumType operator^(EnumType a, EnumType b) noexcept { \
    return static_cast<EnumType>(static_cast<UnderlyingType>(a) ^ static_cast<UnderlyingType>(b)); \
  }                                                  \
  enum class EnumType : UnderlyingType 


#endif //JEMSYS_AGATE2_INTERNAL_FLAGS_HPP
