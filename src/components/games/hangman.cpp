#include "hangman.hpp"

#include "bot.hpp"
#include "utils/fileio.hpp"
#include "utils/parse.hpp"
#include "utils/utils.hpp"

#include <algorithm>
#include <functional>

void qb::Hangman::register_actions(Actions& actions)
{
    using namespace std::placeholders;
    register_all(
        actions,
        std::make_pair("hangman", (ActionCallback)std::bind(&qb::Hangman::run_hangman, this, _1, _2, _3)),
        std::make_pair("guess", (ActionCallback)std::bind(&qb::Hangman::guess_hangman, this, _1, _2, _3)),
        std::make_pair("letter", (ActionCallback)std::bind(&qb::Hangman::letter_hangman, this, _1, _2, _3)));
}

qb::Result qb::Hangman::guess_hangman(const std::string& cmd, const api::Message& msg, Bot& bot)
{
    const auto& channel = msg.channel;
    if (guessed_letters_.empty())
    {
        send_removable_message(bot, "A hangman game is not currently in progress!", channel);
        return qb::Result::ok();
    }
    const auto guess = qb::parse::trim(std::string(std::find(cmd.begin(), cmd.end(), ' '), cmd.end()));
    if (guess_word(guess))
    {
        bot.send("You are correct! The word is " + word_ + "! Game ending...", channel);
        reset();
        return qb::Result::ok();
    }
    bot.send("That is not the correct word! Ouch!", channel);
    return qb::Result::ok();
}

qb::Result qb::Hangman::run_hangman(const std::string& cmd, const api::Message& msg, Bot& bot)
{
    qb::log::point("Starting a hangman game.");
    const auto& channel = msg.channel;
    if (!guessed_letters_.empty())
    {
        bot.send("A hangman game is already in progress!", channel);
        return qb::Result::ok();
    }
    // choose a random word
    const auto words = qb::fileio::get_all();
    // this was actually way easier than expected
    // already had all the framework in place to do this
    word_ = *qb::select_randomly(words.begin(), words.end());
    std::transform(word_.begin(), word_.end(), word_.begin(), tolower);
    guessed_letters_ = " ";
    bot.send("[Hangman] `" + str() + "`", channel);
    qb::log::point("Finished initializing a hangman game.");
    return qb::Result::ok();
}

qb::Result qb::Hangman::letter_hangman(const std::string& cmd, const api::Message& msg, Bot& bot)
{
    const auto& channel = msg.channel;
    if (guessed_letters_.empty())
    {
        bot.send("A hangman game is not currently in progress!", channel);
        return qb::Result::ok();
    }
    const auto guess = qb::parse::trim(std::string(std::find(cmd.begin(), cmd.end(), ' '), cmd.end()));
    auto letters     = qb::parse::concatenate(qb::parse::split(guess), "");
    std::transform(letters.begin(), letters.end(), letters.begin(), tolower);
    guessed_letters_ += letters;
    bot.send("[Hangman] `" + str() + "`", channel);
    return qb::Result::ok();
}

// quick and dirty hangman impl
bool qb::Hangman::guess_letter(std::string l)
{
    if (l.size() != 1) return false;
    if (qb::parse::in(l[0], word_)) return true;
    guessed_letters_ += l;
    return false;
}

bool qb::Hangman::guess_word(std::string w)
{
    return w == word_;
}

std::string qb::Hangman::str()
{
    // I said quick and dirty already, right?
    // This is where things get dirty
    std::string retval = word_;
    std::transform(retval.begin(), retval.end(), retval.begin(),
                   [&](char c) { return (qb::parse::in(c, guessed_letters_) ? c : '#'); });
    // could this be more efficient? yes
    // we'll call this a "todo"
    return retval;
}
