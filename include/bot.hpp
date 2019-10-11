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
    explicit Bot(Flag = Flag::None);
    void start();

private:
    unsigned int hb_interval_ms_ = 0;
};
} // namespace qb

#endif // BOT_HPP
