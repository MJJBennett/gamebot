#ifndef BOT_HPP
#define BOT_HPP

namespace qb
{
class Bot
{
public:
    // Types
    enum class Flag : int
    {
        None     = 0,
        LazyInit = 1
    };

public:
    // Member variables
    explicit Bot(Flag = Flag::None);
    void start();
};
} // namespace qb

#endif // BOT_HPP
