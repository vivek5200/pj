#include <bits/stdc++.h>
#include <unordered_map>
#include <curl/curl.h>
#include <stdio.h>
#include <regex>
#include <thread>
#include <mutex>
#include <chrono>
using namespace std;

mutex m;


void get_page(const char* url, const char* file_name)
{
  CURL* easyhandle = curl_easy_init(); 

  curl_easy_setopt( easyhandle, CURLOPT_URL, url ) ;
  FILE* file = fopen(file_name, "w");
  curl_easy_setopt( easyhandle, CURLOPT_WRITEDATA, file) ;
  curl_easy_perform( easyhandle );
  curl_easy_cleanup( easyhandle );
  fclose(file);
}

vector<string> extract_hyperlinks(string html_file_name )
{
    string html; 
    ifstream read; 
    read.open(html_file_name); 
    while(!read.eof()) 
    {
      if(read.eof()) break; 
      char ch;
      read.get(ch);
      html.push_back(ch); 
    }
    read.close();
    static const regex hl_regex( "<a href=\"(.*?)\">", regex_constants::icase ); 
    vector<string> links; 
 
    copy(sregex_token_iterator(html.begin(), html.end(), hl_regex, 1),sregex_token_iterator(),back_inserter(links));
    return links;
}

void cleanUp(vector<string>&all_links )
{
  vector<string>final_links;
  for(int i=0;i<all_links.size();i++) 
  {
    string one_link = all_links[i];
    string cleaned_link="";
    for(int j=0;j<one_link.length();j++)
    {
      if(one_link[j] == ' ' || one_link[j] == 34 ) break; 
      cleaned_link.push_back(one_link[j]); 
    }
    
    if (regex_match(cleaned_link,regex("((http|https)://)(www.)?[a-zA-Z0-9@:%._\\+~#?&//=]{2,256}\\.[a-z]{2,6}\\b([-a-zA-Z0-9@:%._\\+~#?&//=]*)") )) {
      final_links.push_back(cleaned_link);
    }
  } 
  all_links.clear(); 
  all_links = final_links; 
}

unordered_map<string,bool>visited; 

void dfs_crawler(const char* url,const char* file_path,int depth,int bot_id)
{
  if(depth==4 || visited[url]) return; 
  string s = url;
  visited[s]=1; 
  
  m.lock();
  cout << "Bot_id: "<< bot_id <<"\tLink: "<< url << endl;
  get_page(url, file_path); 

  vector<string> allLinksData  =  extract_hyperlinks(file_path); 
  cleanUp(allLinksData);
  m.unlock();


  for(string i : allLinksData)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    const char *c = i.c_str();
    if(visited[i]!=1) 
      dfs_crawler(c,file_path,depth+1,bot_id);
  }
}

int main()
{

  const char *filename = "1.txt"; 

  thread t1(dfs_crawler,"https://en.wikipedia.org/wiki/Srinivasa_Ramanujan",filename,0,1);
  thread t2(dfs_crawler,"https://github.com/",filename,0,2);
  thread t3(dfs_crawler,"https://codeforces.com/",filename,0,3);

  t1.join();
  t2.join();
  t3.join();
  return 0;
}
