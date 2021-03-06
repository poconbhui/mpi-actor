#ifndef ACTOR_FACTORY_H_
#define ACTOR_FACTORY_H_

#include <vector>
#include <exception>
#include <cstddef>

namespace ActorModel {

/**
 * Factory
 *
 * The factory class is used to enumerate a set of subclasses of some parent
 * class. It can then be used to initialize instances of those subclasses,
 * cast as the parent, using that enumeration.
 */
template<class F>
class Factory {
public:

    /*
     * Return a new instance of an F, initialized as a T.
     *  F *f = create_new<F, T>();
     */
    template<class T>
    static F* create_new(void) {
        return new T;
    }

    typedef F* (create_new_T_signature)(void);

    /*
     * Push a creator function for a given role onto the creator array.
     * The child can then be identified using get_id<Child>().
     */
    template<class T>
    int register_child(void) {
        _F_creators.push_back(create_new<T>);
        return _F_creators.size()-1;
    }

    /*
     * If a role has been registered with the factory, its id in the factory
     * can be found using this function.
     */
    template<class T>
    size_t get_id(void) {
        for(size_t i=0; i<_F_creators.size(); ++i) {
            create_new_T_signature *func = &Factory<F>::create_new<T>;
            if(_F_creators[i] == func) {
                return i;
            }
        }

        // If not found, throw error
        throw FactoryNotFound<T>();
    }

    // Exception class to throw when an unregistered factory is looked up.
    template<class T>
    class FactoryNotFound: public std::exception {
        virtual const char* what() const throw() {
            return "Factory not found!";
        }
    };


    /*
     * If a role_id in the factory is known, a new instance of the role
     * can be generated by using
     *  Actor *actor = create_from_actor_id(actor_id);
     */
    F* create_from_id(int role_id) {
        return _F_creators.at(role_id)();
    }

private:
    std::vector<create_new_T_signature*> _F_creators;
};


}  // namespace ActorModel

#endif  // ACTOR_FACTORY_H_
