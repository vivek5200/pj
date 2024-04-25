#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <regex>
#include <unordered_map>
#include <curl/curl.h>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <deque>

using namespace std;

mutex m;

void get_page(const char* url, const char* file_name)
{
    CURL* easyhandle = curl_easy_init(); 

    curl_easy_setopt(easyhandle, CURLOPT_URL, url);
    FILE* file = fopen(file_name, "w");
    curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, file);
    curl_easy_perform(easyhandle);
    curl_easy_cleanup(easyhandle);
    fclose(file);
}

vector<string> extract_hyperlinks(string html_file_name)
{
    string html;
    ifstream read(html_file_name);
    while (!read.eof())
    {
        if (read.eof())
            break;
        char ch;
        read.get(ch);
        html.push_back(ch);
    }
    read.close();
    static const regex hl_regex("<a href=\"(.*?)\">", regex_constants::icase);
    vector<string> links;

    copy(sregex_token_iterator(html.begin(), html.end(), hl_regex, 1), sregex_token_iterator(), back_inserter(links));
    return links;
}

void cleanUp(vector<string>& all_links)
{
    vector<string> final_links;
    for (int i = 0; i < all_links.size(); i++)
    {
        string one_link = all_links[i];
        string cleaned_link = "";
        for (int j = 0; j < one_link.length(); j++)
        {
            if (one_link[j] == ' ' || one_link[j] == 34)
                break;
            cleaned_link.push_back(one_link[j]);
        }

        if (regex_match(cleaned_link, regex("((http|https)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)")))
        {
            final_links.push_back(cleaned_link);
        }
    }
    all_links.clear();
    all_links = final_links;
}

unordered_map<string, bool> visited;

void dfs_crawler(const char* url, const char* file_path, int depth, int bot_id, atomic<bool>& crawling_finished, deque<string>& crawled_links)
{
    if (depth == 4 || visited[url])
        return;
    string s = url;
    visited[s] = true;

    m.lock();
    cout << "Bot_id: " << bot_id << "\tLink: " << url << endl;
    crawled_links.push_back(url); // Store crawled link
    get_page(url, file_path);

    vector<string> allLinksData = extract_hyperlinks(file_path);
    cleanUp(allLinksData);
    m.unlock();

    for (string i : allLinksData)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        const char* c = i.c_str();
        if (visited[i] != true)
            dfs_crawler(c, file_path, depth + 1, bot_id, crawling_finished, crawled_links);
    }

    if (depth == 0)
        crawling_finished = true; // Mark crawling as finished
}

int main()
{
    // Initialize SFML window
    sf::RenderWindow window(sf::VideoMode(800, 600), "Web Crawler Graphics");

    // Font for text
    sf::Font font;
    if (!font.loadFromFile("Roboto-Medium.ttf"))
    {
        cerr << "Error loading font file!" << endl;
        return 1;
    }

    // Text for displaying crawling status and links
    sf::Text text;
    text.setFont(font);
    text.setCharacterSize(16);
    text.setFillColor(sf::Color::White);
    text.setStyle(sf::Text::Regular);
    text.setPosition(10.f, 10.f);

    // Input URL from the user
    string url;
    cout << "Enter URL: ";
    cin >> url;

    const char* filename = "output.txt";

    // Deque to store crawled links
    deque<string> crawled_links;
    atomic<bool> crawling_finished(false);

    // Launch crawler thread
    thread crawler(dfs_crawler, url.c_str(), filename, 0, 1, ref(crawling_finished), ref(crawled_links));

    // Main SFML event loop
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Clear the window
        window.clear();

        // Draw crawling status and crawled links
        text.setString("Crawling...\n");

        for (const auto& link : crawled_links)
        {
            text.setString(text.getString() + link + "\n");
        }

        if (crawling_finished)
        {
            text.setString(text.getString() + "\nCrawling finished!");
        }

        window.draw(text);

        // Display what was drawn
        window.display();

        // Sleep for a short while to avoid high CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Wait for crawler thread to finish
    crawler.join();

    return 0;
}
