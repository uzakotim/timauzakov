#include "../include/node/libraries.h"
#include "../include/node/colors.h"
#include "../include/node/json/json.h"

static std::mutex lock;
static std::atomic_bool stop_threads = false;

void init_function(const int &id, const std::string &name)
{
    lock.lock();
    auto time = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(time);
    std::cout << FBLU(<< id << ": Init " <<) << FGRN(<< name <<) << FBLU(<< " function: " << std::ctime(&now_time) <<);
    lock.unlock();
}
void input_function(const int &id, const std::string &name, const int &delay)
{
    init_function(id, name);
    struct termios old_tio, new_tio;
    unsigned char c;

    /* get the terminal settings for stdin */
    tcgetattr(STDIN_FILENO, &old_tio);
    /* we want to keep the old setting to restore them a the end */
    new_tio = old_tio;
    /* disable canonical mode (buffered i/o) and local echo */
    new_tio.c_lflag &= (~ICANON & ~ECHO);
    /* set the new settings immediately */
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
    while (1)
    {
        c = getchar();
        if (c == 'q')
            break;
        std::cout << c << '\n';
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    };
    /* restore the former settings */
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
    stop_threads = true;
    return;
}
void thread_function(const int &id, const std::string &name, const int &delay)
{
    //* --------------------------------
    //  NODE function
    //* --------------------------------
    init_function(id, name);
    //* --------------------------------
    static std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
    static std::chrono::system_clock::time_point start;
    const double i = 20;
    Json::Value value_obj;
    value_obj["signal"] = i;
    double elapsed_seconds = 0;

    while (!stop_threads)
    {
        // --------------------------------
        lock.lock();
        start = std::chrono::system_clock::now();
        elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(start - end).count();
        std::cout << "dt[s] " << elapsed_seconds << '\n';
        end = std::chrono::system_clock::now();
        //  ------------------------------------
        // Write data
        std::ofstream outputFileStream;
        outputFileStream.open("../data/data.json");
        if (!outputFileStream)
        {
            // file couldn't be opened
            std::cerr << "Error: file could not be opened" << std::endl;
            exit(1);
        }
        outputFileStream << value_obj << std::endl;
        outputFileStream.close();
        lock.unlock();
        // --------------------------------
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }
}
void checker_function(const int &id, const std::string &name, const int &delay)
{
    //* --------------------------------
    init_function(id, name);
    static double signal_value = 0;
    //* -------------------------------------
    while (!stop_threads)
    {
        // -----------------------------------
        // Reading file
        lock.lock();
        std::ifstream data_file("../data/data.json", std::ifstream::binary);
        Json::Value data;
        data_file >> data;
        data_file.close();
        lock.unlock();
        // -----------------------------------
        std::cout << "measured signal: " << data["signal"] << '\n';
        signal_value = data["signal"].asDouble();
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }
}
int main(int argc, char **argv)
{
    std::cout << "Concurrent program started" << std::endl;
    std::cout << FYEL("Press \"q\" to end testing\n");
    std::thread th1(thread_function, 1, "thread", frequency_to_milliseconds(1));   // frequency in Hz
    std::thread ch1(checker_function, 2, "checker", frequency_to_milliseconds(1)); // frequency in Hz
    std::thread ip1(input_function, 3, "input", frequency_to_milliseconds(1));     // frequency in Hz
    th1.join();
    ch1.join();
    ip1.join();
    return 0;
}