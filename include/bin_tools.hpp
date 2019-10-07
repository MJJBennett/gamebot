#ifndef GAMEBOT_BIN_TOOLS_HPP
#define GAMEBOT_BIN_TOOLS_HPP

namespace qb
{
template<typename T, typename L, typename R>
T bitwise_and(L l, R r){
    return (static_cast<T>(l) & static_cast<T>(r));
}
}

#endif // GAMEBOT_BIN_TOOLS_HPP
