#ifndef QB_HANGMAN_HPP
#define QB_HANGMAN_HPP

#include "../component.hpp"

#include <string>

namespace qb
{
class Hangman : public Component
{
    constexpr static size_t MAX_GUESSES = 5;

public:
    // this is very much quick-and-dirty
    // quick implementation for fun

    qb::Result run_hangman(const std::string& cmd, const api::Message& msg, Bot& bot);
    qb::Result guess_hangman(const std::string& cmd, const api::Message& msg, Bot& bot);
    qb::Result letter_hangman(const std::string& cmd, const api::Message& msg, Bot& bot);

    void register_actions(Actions<>& actions) override;

private:
    void reset()
    {
        guessed_letters_.clear();
        word_.clear();
        max_guesses_ = MAX_GUESSES;
    }

    bool guess_letter(std::string l);
    bool guess_word(std::string w);

    std::string str();

    // This always contains a space during runtime,
    // so we can use it being empty to indicate that
    // we are not running a hangman session.
    std::string guessed_letters_;
    std::string word_;

    size_t max_guesses_{MAX_GUESSES};
};
} // namespace qb

#endif // QB_HANGMAN_HPP
