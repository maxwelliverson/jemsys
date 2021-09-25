//
// Created by maxwe on 2021-09-19.
//

#ifndef JEMSYS_INTERNAL_HANDLES_HPP
#define JEMSYS_INTERNAL_HANDLES_HPP

#include "dictionary.hpp"
#include "pointer_set.hpp"

#include <deque>
#include <vector>
#include <chrono>
#include <map>

namespace jem{

  using clock = std::chrono::high_resolution_clock;
  using timestamp_t = typename clock::time_point;

  enum {
    object_is_named    = 0x1,
    object_is_shared   = 0x2,
    object_is_root     = 0x4,
    object_is_imported = 0x8
  };
  enum {
    disconnect_is_unexpected = 0x1,
    disconnect_is_indirect   = 0x2
  };

  using object_flags_t = jem_flags32_t;
  using object_disconnect_flags_t = jem_flags32_t;

  struct object_descriptor;

  class object_base {
  protected:
    ~object_base() = default;
  public:
    object_descriptor* descriptor;
    jem_u64_t          typeId;
    object_descriptor* owner;
    jem_u32_t          generation;
    jem_u32_t          refCount;
    object_flags_t     flags;


    virtual void destroy(int reason) noexcept = 0;
  };

  using object_disconnect_callback_t = void(*)(object_base* user, object_base* target, int reason, object_disconnect_flags_t flags, void* userData);
  using object_dtor_callback_t       = void(*)(object_base* object, int reason, object_disconnect_flags_t flags, void* userData);

  struct object_connection_descriptor {
    object_connection_descriptor* next;
    object_connection_descriptor* prev;
    object_descriptor*            client;
    object_descriptor*            server;
    object_disconnect_callback_t  callback;
    void*                         userData;
  };



  struct object_descriptor {

    object_base*                       object;
    timestamp_t                        creationTimestamp;

    pointer_set<object_descriptor*, 1> children;
    std::map<object_descriptor*, object_connection_descriptor> usedByList;
    pointer_set<object_descriptor*, 4> useList;

    object_dtor_callback_t dtorCallback;
    void*                  dtorUserData;
  };

  class object_manager {

    object_descriptor               rootObject;
    dictionary<object_descriptor>   namedObjects;
    pointer_set<object_descriptor*> anonymousObjects;
    fixed_size_pool                 descriptorPool;
    jem_size_t                      totalObjectCount;

    int                             manualDisconnectReason;
    int                             parentDestroyedReason;



    inline static std::string_view object_name(object_descriptor* desc) noexcept {
      assert( desc->object->flags & object_is_named );
      return reinterpret_cast<dictionary_entry<object_descriptor>*>(desc)->key();
    }

    void do_destroy_object(object_descriptor* desc, int reason, object_disconnect_flags_t flags) noexcept {
      desc->dtorCallback(desc->object, reason, flags, desc->dtorUserData);

      assert(totalObjectCount > 0);

      if ( desc->object->flags & object_is_named ) {
        assert(!namedObjects.empty());
        assert(namedObjects.contains(object_name(desc)));
        namedObjects.erase(object_name(desc));
      }
      else if ( desc->object->flags & object_is_root ) [[unlikely]] {

      }
      else {
        assert(!anonymousObjects.empty());
        assert(anonymousObjects.contains(desc));
        anonymousObjects.erase(desc);
        void* descMem = desc;
        desc->~object_descriptor();
        descriptorPool.free_block(descMem);
      }

      --totalObjectCount;
    }

    void do_disconnect_client(object_descriptor* client, object_descriptor* server, int reason, object_disconnect_flags_t flags = 0x0) noexcept {
      do_disconnect(client, server, reason, flags);
      server->usedByList.erase(client);
      if ( server->usedByList.empty() && server->object->refCount == 0 )
        do_destroy_object(server, reason, flags);
    }
    static void do_disconnect_server(object_descriptor* client, object_descriptor* server, int reason, object_disconnect_flags_t flags = 0x0) noexcept {
      do_disconnect(client, server, reason, flags | disconnect_is_unexpected);
      client->useList.erase(server);
    }
    static void do_disconnect_server(const object_connection_descriptor& connection, int reason, object_disconnect_flags_t flags = 0x0) noexcept {
      do_disconnect(connection, reason, flags | disconnect_is_unexpected);
      connection.client->useList.erase(connection.server);
    }

    static void do_disconnect(object_descriptor* client, object_descriptor* server, int reason, object_disconnect_flags_t flags) noexcept {
      auto entry = server->usedByList.find(client);
      assert( entry != server->usedByList.end() );
      auto&& [clientCopy, connection] = *entry;
      assert(clientCopy == client);
      assert(client == connection.client);
      assert(server == connection.server);
      do_disconnect(connection, reason, flags);
    }
    static void do_disconnect(const object_connection_descriptor& connection, int reason, object_disconnect_flags_t flags) noexcept {
      if ( connection.callback )
        connection.callback(connection.client->object, connection.server->object, reason, flags, connection.userData);
    }

    void do_force_destroy(object_descriptor* obj, int reason, object_disconnect_flags_t flags = 0) noexcept {

      for ( auto child : obj->children ) {
        assert( child->object->owner == obj );
        do_force_destroy(child, reason, flags | disconnect_is_indirect);
      }

      for ( auto&& [client, connection] : obj->usedByList ) {
        assert(connection.client == client);
        assert(connection.server == obj);
        do_disconnect_server(connection, reason, flags);
      }

      for ( auto server : obj->useList)
        do_disconnect_client(obj, server, reason, flags);

      if ( !(flags & disconnect_is_indirect) )
        obj->object->owner->children.erase(obj);

      do_destroy_object(obj, reason, flags);
    }

  public:


    object_descriptor* register_anonymous_object(object_descriptor* owner, object_base* object, jem_u64_t typeId, bool isShared) noexcept {
      return register_anonymous_object(owner, object, typeId, isShared, nullptr, nullptr);
    }
    object_descriptor* register_anonymous_object(object_descriptor* owner, object_base* object, jem_u64_t typeId, bool isShared, object_dtor_callback_t dtorCallback, void* dtorUserData) noexcept {
      object_descriptor* newDescriptor;

      if ( !owner )
        owner = &rootObject;

      auto newDescMem = descriptorPool.alloc_block();
      newDescriptor = new (newDescMem) object_descriptor{
        .object     = object,
        /*.typeId     = typeId,
        .owner      = owner,
        .generation = owner->object->generation + 1,
        .refCount   = 1,
        .flags      = isShared ? object_is_shared : 0u,*/
        .creationTimestamp = clock::now(),
        .dtorCallback = dtorCallback,
        .dtorUserData = dtorUserData
      };
      object->typeId = typeId;
      object->owner  = owner;
      object->generation = owner->object->generation + 1;
      object->refCount = 1;
      object->flags = isShared ? object_is_shared : 0u;

      anonymousObjects.insert(newDescriptor);
      object->descriptor = newDescriptor;
      owner->children.insert(newDescriptor);

      ++totalObjectCount;

      return newDescriptor;
    }
    object_descriptor* register_object(object_descriptor* owner, object_base* object, jem_u64_t typeId, std::string_view name, bool isShared) noexcept {
      return register_object(owner, object, typeId, name, isShared, nullptr, nullptr);
    }
    object_descriptor* register_object(object_descriptor* owner, object_base* object, jem_u64_t typeId, std::string_view name, bool isShared, object_dtor_callback_t dtorCallback, void* dtorUserData) noexcept {
      object_descriptor* newDescriptor;

      auto&& [entry, isNew] = namedObjects.try_emplace(name);

      if ( !isNew )
        return nullptr;

      if ( !owner )
        owner = &rootObject;

      newDescriptor = &entry->get();

      newDescriptor->object = object;
      /*newDescriptor->typeId  = typeId;
      newDescriptor->owner   = owner;
      newDescriptor->generation = owner->generation + 1;
      newDescriptor->refCount = 1;
      newDescriptor->flags = object_is_named | (isShared ? object_is_shared : 0u);*/
      object->typeId  = typeId;
      object->owner   = owner;
      object->generation = owner->object->generation + 1;
      object->refCount = 1;
      object->flags = object_is_named | (isShared ? object_is_shared : 0u);
      newDescriptor->creationTimestamp = clock::now();
      newDescriptor->dtorCallback = dtorCallback;
      newDescriptor->dtorUserData = dtorUserData;

      owner->children.insert(newDescriptor);
      object->descriptor = newDescriptor;

      ++totalObjectCount;

      return newDescriptor;
    }

    object_descriptor* lookup(std::string_view name) const noexcept {
      auto result = namedObjects.find(name);
      if ( result == namedObjects.end() )
        return nullptr;
      return &const_cast<object_descriptor&>(result->get());
    }


    bool connect(object_base* client, object_base* server, object_disconnect_callback_t callback, void* userData) const noexcept {
      auto clientDesc = client->descriptor;
      auto serverDesc = server->descriptor;

      auto [position, isNew] = clientDesc->useList.insert(serverDesc);

      if ( !isNew )
        return false;

      auto&& [connectionPos, connectionIsNew] = serverDesc->usedByList.try_emplace(clientDesc);
      assert( connectionIsNew );
      auto& [clientDescCopy, connection] = *connectionPos;
      assert( clientDescCopy == clientDesc );
      connection.client = clientDesc;
      connection.server = serverDesc;
      connection.callback = callback;
      connection.userData = userData;

      return true;
    }

    void disconnect_client(object_base* client, object_base* server, int reason) noexcept {
      auto clientDesc = client->descriptor;
      auto serverDesc = server->descriptor;


    }
    void disconnect_server(object_base* client, object_base* server, int reason) noexcept {

    }


    void force_destroy(object_base* object, int reason) noexcept {
      do_force_destroy(object->descriptor, reason);
    }

    void import_object(object_base* object) noexcept;
    void export_object(object_base* object) noexcept;
  };
}


#endif//JEMSYS_INTERNAL_HANDLES_HPP
