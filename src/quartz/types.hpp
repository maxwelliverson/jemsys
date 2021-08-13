//
// Created by maxwe on 2021-06-09.
//

#ifndef JEMSYS_QUARTZ_INTERNAL_TYPES_HPP
#define JEMSYS_QUARTZ_INTERNAL_TYPES_HPP

#include "ipc/span.hpp"
#include "ipc/string.hpp"

#include "mm.hpp"


namespace qtz::types{

  enum status_code{
    success,
    error_null_type,
    error_bad_flags_on_base,
    error_inherit_from_final,
    error_inherit_from_non_aggregate,
    error_inherit_from_opaque,
    error_opaque_instance,
    error_abstract_instance,
    error_member_name_clash
  };

  class status{
    inline constexpr static size_t StorageSize  = 24;
    inline constexpr static size_t StorageAlign = 8;
  public:



  private:
    std::aligned_storage_t<StorageSize, StorageAlign> storage;
    status_code code;
  };

  namespace impl{
    namespace {
#if JEM_system_windows
      struct __single_inheritance S;
      struct __multiple_inheritance M;
      struct __virtual_inheritance V;
#else
      struct polymorphic_inheritance{
        int member;
        virtual void member_function();
      };
      struct S : polymorphic_inheritance{
        int  second_member;
        void second_member_function();
      };
      struct secondary_inheritance{
        int second_member;
        virtual void second_member_function();
      };
      struct M : polymorphic_inheritance, secondary_inheritance{};
      struct poly_a : virtual polymorphic_inheritance{
        void member_function() override;
      };
      struct poly_b : virtual polymorphic_inheritance{
        //void member_function() override;
      };
      struct V : poly_a, poly_b{ };
#endif
    }// namespace
  }


  struct descriptor;

  enum flags_t : jem_u16_t {
    default_flags             = 0x0000,
    const_qualified_flag      = 0x0001,
    volatile_qualified_flag   = 0x0002,
    mutable_qualified_flag    = 0x0004,
    public_visibility_flag    = 0x0008,
    private_visibility_flag   = 0x0010,
    protected_visibility_flag = 0x0020,
    is_virtual_flag           = 0x0040,
    is_abstract_flag          = 0x0080,
    is_override_flag          = 0x0100,
    is_final_flag             = 0x0200,
    is_bitfield_flag          = 0x0400,
    is_noexcept_flag          = 0x0800,
    is_variadic_flag          = 0x1000,
    is_opaque_flag            = 0x2000
  };
  enum kind_t  : jem_u8_t {
    void_type,
    boolean_type,
    integral_type,
    floating_point_type,
    pointer_type,
    array_type,
    function_type,
    member_pointer_type,
    member_function_pointer_type,
    aggregate_type,
    enumerator_type
  };

  using  type_t = ipc::offset_ptr<const descriptor>;
  using  type_id_t = const descriptor*;
  struct type_id_ref_t{
    type_id_t type;
    flags_t flags;
  };
  struct aggregate_member_id_t{
    type_id_t        type;
    flags_t          flags;
    jem_u8_t         bitfield_bits;
    jem_u32_t        alignment;
    std::string_view name;
  };
  struct enumeration_id_t{
    std::string_view name;
    jem_u64_t        value;
  };


  static std::string_view allocate_string(std::string_view name) noexcept;
  static void*            allocate_array(jem_size_t objectSize, jem_size_t objectCount) noexcept;
  template <typename T>
  inline static T* allocate_array(jem_size_t count) noexcept {
    return static_cast<T*>(allocate_array(sizeof(T), count));
  }
  template <typename T>
  static std::span<ipc::offset_ptr<T>> allocate_pointer_span(std::span<T*> pointers) noexcept;

  inline static void align_offset(jem_size_t& offset, jem_size_t alignment) noexcept {
    const jem_size_t lower_mask = alignment - 1;
    --offset;
    offset |= lower_mask;
    ++offset;
  }
  inline static void update_alignment(jem_size_t& alignment, jem_size_t newAlign) noexcept {
    alignment = std::max(alignment, newAlign);
  }


  struct void_params_t{};
  struct boolean_params_t{};
  struct integral_params_t{
    std::string_view name;
    jem_u32_t        size;
    jem_u32_t        alignment;
    bool             is_signed;
  };
  struct floating_point_params_t{
    std::string_view name;
    jem_u32_t        size;
    jem_u32_t        alignment;
    bool             is_signed;
    jem_u16_t        exponent_bits;
    jem_u32_t        mantissa_bits;
  };
  struct pointer_params_t{
    type_id_ref_t pointee_type;
  };
  struct reference_params_t{
    type_id_ref_t pointee_type;
  };
  struct array_params_t{
    type_id_ref_t elements;
    jem_u32_t     count;
  };
  struct function_params_t{
    type_id_ref_t                  ret;
    std::span<const type_id_ref_t> args;
    flags_t                        flags;
  };
  struct member_pointer_params_t{
    type_id_ref_t pointee;
    type_id_t     aggregate_type;
  };
  struct member_function_pointer_params_t{
    type_id_ref_t function;
    type_id_t     aggregate_type;
  };
  struct aggregate_params_t{
    flags_t                                flags;
    jem_u32_t                              alignment;
    std::string_view                       name;
    std::span<const type_id_ref_t>         bases;
    std::span<const aggregate_member_id_t> members;
  };
  struct enumerator_params_t{
    std::string_view                  name;
    type_id_t                         underlying_type;
    std::span<const enumeration_id_t> entries;
    bool                              is_bitflag;
  };

  struct platform_constants_t{
    jem_size_t pointer_size;
    jem_size_t pointer_alignment;
    jem_size_t si_member_pointer_size;
    jem_size_t mi_member_pointer_size;
    jem_size_t vi_member_pointer_size;
    jem_size_t si_member_pointer_alignment;
    jem_size_t mi_member_pointer_alignment;
    jem_size_t vi_member_pointer_alignment;
    jem_size_t si_member_function_pointer_size;
    jem_size_t mi_member_function_pointer_size;
    jem_size_t vi_member_function_pointer_size;
    jem_size_t si_member_function_pointer_alignment;
    jem_size_t mi_member_function_pointer_alignment;
    jem_size_t vi_member_function_pointer_alignment;
  };

  static constexpr platform_constants_t platform_constants{
    .pointer_size                         = sizeof(void*),
    .pointer_alignment                    = alignof(void*),
    .si_member_pointer_size               = sizeof(int impl::S::*),
    .mi_member_pointer_size               = sizeof(int impl::M::*),
    .vi_member_pointer_size               = sizeof(int impl::V::*),
    .si_member_pointer_alignment          = alignof(int impl::S::*),
    .mi_member_pointer_alignment          = alignof(int impl::M::*),
    .vi_member_pointer_alignment          = alignof(int impl::V::*),
    .si_member_function_pointer_size      = sizeof(int (impl::S::*)()),
    .mi_member_function_pointer_size      = sizeof(int (impl::M::*)()),
    .vi_member_function_pointer_size      = sizeof(int (impl::V::*)()),
    .si_member_function_pointer_alignment = alignof(int (impl::S::*)()),
    .mi_member_function_pointer_alignment = alignof(int (impl::M::*)()),
    .vi_member_function_pointer_alignment = alignof(int (impl::V::*)())
  };


  class descriptor{

    inline static constexpr jem_u16_t QualifierMask  = jem_u16_t(const_qualified_flag | volatile_qualified_flag);
    inline static constexpr jem_u16_t VisibilityMask = jem_u16_t(public_visibility_flag | protected_visibility_flag | private_visibility_flag);

    using  type_t = ipc::offset_ptr<const descriptor>;
    struct type_ref_t{
      type_t  type  = nullptr;
      flags_t flags = default_flags;
    };
    struct aggregate_member_t{
      type_t           type;
      flags_t          flags;
      jem_u8_t         bitfield_bits;
      jem_u8_t         bitfield_offset;
      jem_u32_t        offset;
      ipc::string      name;
    };
    struct aggregate_base_t {
      type_t    type;
      flags_t   flags;
      jem_u32_t offset;
    };
    struct enumeration_t{
      ipc::string name;
      jem_u64_t   value;
    };

    enum inheritance_kind : jem_u8_t { single_inheritance, multi_inheritance, virtual_inheritance };

    kind_t     kind;
    jem_size_t size;
    jem_size_t alignment;

    union{
      char default_init = '\0';
      struct {
        ipc::string name;
        bool        is_signed;
      } integral;
      struct {
        ipc::string name;
        bool        is_signed;
        jem_u16_t   exponent_bits;
        jem_u32_t   mantissa_bits;
      } floating_point;
      struct {
        type_ref_t pointee;
      } pointer;
      struct {
        struct {
          type_t     type;
          flags_t    flags;
          bool       is_bounded;
          jem_u32_t  count;
        } element;
      } array;
      struct {
        type_ref_t                  ret;
        ipc::span<const type_ref_t> args;
        bool                        is_variadic;
        bool                        is_noexcept;
      } function;
      struct {
        type_ref_t pointee;
        type_t aggregate_type;
      } member_pointer;
      struct {
        type_ref_t function;
        type_t     aggregate_type;
      } member_function_pointer;
      struct {
        ipc::string                         name;
        ipc::span<const aggregate_base_t>   bases;
        ipc::span<const aggregate_member_t> members;
        bool                                is_polymorphic;
        bool                                is_abstract;
        bool                                is_final;
        inheritance_kind                    inheritance;
      } aggregate;
      struct {
        ipc::string                    name;
        type_t                         underlying_type;
        ipc::span<const enumeration_t> entries;
        bool                           is_bitflag;
      } enumerator;
    } mem = {};

  public:

    descriptor() = default;
    explicit descriptor(void_params_t) noexcept
        : kind(void_type),
          size(0),
          alignment(0)
    {}
    explicit descriptor(boolean_params_t) noexcept
        : kind(boolean_type),
          size(1),
          alignment(1)
    {}
    explicit descriptor(const integral_params_t& params) noexcept
        : kind(integral_type),
          size(params.size),
          alignment(params.alignment),
          mem{
            .integral = {
              .name = allocate_string(params.name),
              .is_signed = params.is_signed
            }
          }
    {}
    explicit descriptor(const floating_point_params_t& params) noexcept
        : kind(floating_point_type),
          size(params.size),
          alignment(params.alignment),
          mem{
            .floating_point = {
              .name = allocate_string(params.name),
              .is_signed = params.is_signed,
              .exponent_bits = params.exponent_bits,
              .mantissa_bits = params.mantissa_bits
            }
          }
    {}
    explicit descriptor(const pointer_params_t& params) noexcept
        : kind(pointer_type),
          size(platform_constants.pointer_size),
          alignment(platform_constants.pointer_alignment),
          mem{
            .pointer = {
              .pointee = {
                .type  = params.pointee_type.type,
                .flags = params.pointee_type.flags
              }
            }
          }
    {}
    explicit descriptor(const array_params_t& params) noexcept
        : kind(array_type),
          size(params.count == 0 ? 0 : (params.elements.type->size * params.count)),
          alignment(params.elements.type->alignment),
          mem {
            .array = {
              .element = {
                .type       = params.elements.type,
                .flags      = params.elements.flags,
                .is_bounded = params.count != 0,
                .count      = params.count
              }
            }
          }
    {}
    explicit descriptor(const function_params_t& params) noexcept
        : kind(function_type),
          size(0),
          alignment(1),
          mem {
            .function = {
              .ret = {
                .type  = params.ret.type,
                .flags = params.ret.flags
              },
              .is_variadic = bool(params.flags & is_variadic_flag),
              .is_noexcept = bool(params.flags & is_noexcept_flag)
            }
          }
    {
      const jem_size_t argCount = params.args.size();
      auto* argsPtr = allocate_array<type_ref_t>(argCount);
      for ( jem_size_t i = 0; i < argCount; ++i ) {
        argsPtr[i].type  = params.args[i].type;
        argsPtr[i].flags = params.args[i].flags;
      }
      mem.function.args = { argsPtr, argCount };
    }
    explicit descriptor(const member_pointer_params_t& params) noexcept
        : kind(member_pointer_type),
          size(member_pointer_size(platform_constants, params.aggregate_type->mem.aggregate.inheritance)),
          alignment(member_pointer_alignment(platform_constants, params.aggregate_type->mem.aggregate.inheritance)),
          mem {
            .member_pointer = {
              .pointee = {
                .type  = params.pointee.type,
                .flags = params.pointee.flags
              },
              .aggregate_type = params.aggregate_type
            }
          }
    {}
    explicit descriptor(const member_function_pointer_params_t& params) noexcept
        : kind(member_function_pointer_type),
          size(member_function_pointer_size(platform_constants, params.aggregate_type->mem.aggregate.inheritance)),
          alignment(member_function_pointer_alignment(platform_constants, params.aggregate_type->mem.aggregate.inheritance)),
          mem {
            .member_function_pointer = {
              .function = {
                .type  = params.function.type,
                .flags = params.function.flags
              },
              .aggregate_type = params.aggregate_type
            }
          }
    {}
    explicit descriptor(const aggregate_params_t& params) noexcept
        : kind(aggregate_type),
          size(0),
          alignment(params.alignment),
          mem{
            .aggregate = {
              .name = params.name,
              .is_polymorphic = bool(params.flags & is_virtual_flag),
              .is_abstract    = bool(params.flags & is_abstract_flag),
              .is_final       = bool(params.flags & is_final_flag),
              .inheritance    = single_inheritance
            }
          }
    {
      jem_size_t offset = 0;
      jem_size_t bitfield_offset = 0;


      const jem_size_t baseCount   = params.bases.size();
      const jem_size_t memberCount = params.members.size();
      auto* const      basePtr     = allocate_array<aggregate_base_t>(baseCount);
      auto* const      memberPtr   = allocate_array<aggregate_member_t>(memberCount);

      if ( baseCount > 1 ) {
        mem.aggregate.inheritance = multi_inheritance;
      }

      for ( jem_size_t i = 0; i < baseCount;   ++i ) {
        basePtr[i].type   = params.bases[i].type;
        basePtr[i].flags  = params.bases[i].flags;

        align_offset(offset, params.bases[i].type->alignment);

        basePtr[i].offset = offset;
        offset += params.bases[i].type->size;
        update_alignment(alignment, params.bases[i].type->alignment);
        if ( basePtr[i].flags & is_virtual_flag )
          mem.aggregate.inheritance = virtual_inheritance;
        else
          mem.aggregate.inheritance = std::max(mem.aggregate.inheritance, basePtr[i].type->mem.aggregate.inheritance);
      }

      if ( mem.aggregate.is_polymorphic && offset == 0 ) {
        // vptr
        offset += sizeof(void*);
        update_alignment(alignment, alignof(void*));
      }

      for ( jem_size_t i = 0; i < memberCount; ++i ) {
        const jem_size_t member_alignment = params.members[i].alignment ?: params.members[i].type->alignment;
        const jem_size_t type_bits        = params.members[i].type->size * CHAR_BIT;

        memberPtr[i].name            = allocate_string(params.members[i].name);
        memberPtr[i].type            = params.members[i].type;
        memberPtr[i].flags           = params.members[i].flags;

        if ( params.members[i].flags & is_bitfield_flag ) {
          memberPtr[i].bitfield_bits   = params.members[i].bitfield_bits;
          if ( bitfield_offset + memberPtr[i].bitfield_bits > type_bits ) {
            bitfield_offset = 0;
            align_offset(offset, member_alignment);
            memberPtr[i].offset = offset;
            offset += memberPtr[i].bitfield_bits / 8;
          }

          memberPtr[i].bitfield_offset = bitfield_offset;
          bitfield_offset += memberPtr[i].bitfield_bits;
        }
        else {
          align_offset(offset, member_alignment);
          memberPtr[i].offset = offset;
          offset += memberPtr[i].type->size;
          memberPtr[i].bitfield_bits = 0;
          memberPtr[i].bitfield_offset = 0;
          bitfield_offset = 0;
        }

        update_alignment(alignment, member_alignment);
      }

      align_offset(offset, alignment);
      size = offset;

      mem.aggregate.bases   = { basePtr, baseCount };
      mem.aggregate.members = { memberPtr, memberCount };
    }
    explicit descriptor(const enumerator_params_t& params) noexcept
        : kind(enumerator_type),
          size(params.underlying_type->size),
          alignment(params.underlying_type->alignment),
          mem{
            .enumerator = {
              .name            = params.name,
              .underlying_type = params.underlying_type,
              .entries         = {},
              .is_bitflag      = params.is_bitflag
            }
          }
    {
      const jem_size_t entryCount = params.entries.size();
      auto* entryPtr = allocate_array<enumeration_t>(entryCount);

      for ( jem_size_t i = 0; i < entryCount; ++i ) {
        entryPtr[i].value = params.entries[i].value;
        entryPtr[i].name  = allocate_string(params.entries[i].name);
      }

      mem.enumerator.entries = { entryPtr, entryCount };
    }




  private:

    JEM_nodiscard inline static jem_size_t member_pointer_size(const platform_constants_t& constants, inheritance_kind inheritance) noexcept {
      return *(&constants.si_member_pointer_size + jem_size_t(inheritance));
    }
    JEM_nodiscard inline static jem_size_t member_pointer_alignment(const platform_constants_t& constants, inheritance_kind inheritance) noexcept {
      return *(&constants.si_member_pointer_alignment + jem_size_t(inheritance));
    }
    JEM_nodiscard inline static jem_size_t member_function_pointer_size(const platform_constants_t& constants, inheritance_kind inheritance) noexcept {
      return *(&constants.si_member_function_pointer_size + jem_size_t(inheritance));
    }
    JEM_nodiscard inline static jem_size_t member_function_pointer_alignment(const platform_constants_t& constants, inheritance_kind inheritance) noexcept {
      return *(&constants.si_member_function_pointer_alignment + jem_size_t(inheritance));
    }
  };


}

#endif//JEMSYS_QUARTZ_INTERNAL_TYPES_HPP
