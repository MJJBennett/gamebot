#ifndef QB_COMPONENT_HPP
#define QB_COMPONENT_HPP

#include "action.hpp"

namespace qb{ 
class Component {
    public:
        virtual void register_actions(Actions& actions) = 0;

};


}

#endif // QB_COMPONENT_HPP
