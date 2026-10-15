#pragma once
// Port of client/src/objects/objectPool.ts: AbstractObject + Pool<T> + the
// id->object creator used to apply UpdateMsg object deltas to barns.
#include "../../net/ObjectSerializeFns.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace surv {

class GameWorld;
using Ctx = GameWorld;

class AbstractObject {
public:
    uint16_t __id = 0;
    uint8_t __type = ObjectType_Invalid;
    bool active = false;

    virtual ~AbstractObject() = default;
    virtual void m_init() {}
    virtual void m_free() {}
    // fullUpdate=true when this came from a FullObject, false for a PartObject.
    virtual void m_updateData(const ObjectData& data, bool fullUpdate, bool isNew, Ctx& ctx) = 0;
};

template <class T>
class Pool {
public:
    static_assert(std::is_base_of<AbstractObject, T>::value, "Pool<T> requires AbstractObject");

    T* m_alloc() {
        T* obj = nullptr;
        for (auto& p : _pool) {
            if (!p->active) {
                obj = p.get();
                break;
            }
        }
        if (!obj) {
            _pool.push_back(std::make_unique<T>());
            obj = _pool.back().get();
        }
        obj->active = true;
        obj->m_init();
        _activeCount++;
        rebuild();
        return obj;
    }

    void m_free(AbstractObject* obj) {
        obj->m_free();
        obj->active = false;
        _activeCount--;
        rebuild();
    }

    std::vector<T*>& m_getPool() {
        rebuild();
        return _raw;
    }

    const std::vector<T*>& m_getPool() const {
        rebuild();
        return _raw;
    }

    int activeCount() const { return _activeCount; }

    // Deactivates every object without shrinking the pool (Pool<T> keeps its
    // allocations for reuse). Used on a match change, where the barn/creator
    // tables are rebuilt but the objects themselves are recycled.
    void clear() {
        for (auto& p : _pool) {
            p->m_free();
            p->active = false;
        }
        _activeCount = 0;
        rebuild();
    }

private:
    void rebuild() const {
        _raw.clear();
        _raw.reserve(_pool.size());
        for (auto& p : _pool) {
            _raw.push_back(p.get());
        }
    }

    std::vector<std::unique_ptr<T>> _pool;
    mutable std::vector<T*> _raw;
    int _activeCount = 0;
};

// Port of Creator: routes full/part/delete object messages to the right pool.
class ObjectCreator {
public:
    template <class T>
    void registerPool(uint8_t type, Pool<T>* pool) {
        _alloc[type] = [pool]() -> AbstractObject* { return pool->m_alloc(); };
    }

    AbstractObject* getObjById(uint16_t id) {
        auto it = _idToObj.find(id);
        return it == _idToObj.end() ? nullptr : it->second;
    }

    AbstractObject* updateObjFull(uint8_t type, uint16_t id, const ObjectData& data, Ctx& ctx);
    void updateObjPart(uint16_t id, const ObjectData& data, Ctx& ctx);
    void deleteObj(uint16_t id);

    const std::vector<AbstractObject*>& objects() const { return _order; }

    // objectPool.ts `Creator.reset()`: drop every id->object mapping so a new
    // match never resolves ids against a previous match's pool entries. The
    // pools keep their objects (they are reused), but a stale `_order` entry
    // would make the world hand out an object a pooled reload has since
    // re-initialised, so it is cleared here too.
    void reset() {
        _idToObj.clear();
        _order.clear();
    }

private:
    std::unordered_map<uint8_t, std::function<AbstractObject*()>> _alloc;
    std::unordered_map<uint16_t, AbstractObject*> _idToObj;
    std::vector<AbstractObject*> _order;
};

} // namespace surv
