//
// Created by maxwe on 2021-11-22.
//

#include "objects.hpp"


JEM_forceinline jem_size_t align_to(jem_size_t size, jem_size_t align) noexcept {
  return ((size - 1) | (align - 1)) + 1;
}


Agt::ObjectManager::ObjectManager(jem_u32_t processId, AgtSize pageSize) noexcept {}

Agt::ObjectManager::~ObjectManager() {}


bool Agt::ObjectManager::registerPrivate(Object* object) noexcept {}

bool Agt::ObjectManager::registerShared(Object* object) noexcept {}

Agt::Object* Agt::ObjectManager::lookup(Id id) const noexcept {}

Agt::Object* Agt::ObjectManager::unsafeLookup(Id id) const noexcept {}

void Agt::ObjectManager::release(Id id) noexcept {}

void Agt::ObjectManager::release(Object* obj) noexcept {}
