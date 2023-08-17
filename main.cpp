#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

using namespace std;
using filesystem::path;

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

// напишите эту функцию

bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories){
    
    ifstream fout(in_file.string());
    if(!fout.is_open()) return false;

    ofstream file_(out_file.string(), std::ios::app);
    if(!file_.is_open()) return false;

    static regex incl_reg1("\\s*#\\s*include\\s*\"([^\"]*)\"\\s*");
    static regex incl_reg2("\\s*#\\s*include\\s*<([^>]*)>\\s*");
    smatch m;

    string s;
    int string_number = 1;
    while(getline(fout, s)){
        bool emplaced = false; 

        if(std::regex_search(s, m, incl_reg1)){
            bool is_found = false;
            for(const auto& dir_entry : filesystem::directory_iterator(in_file.parent_path())){
                string sup_str1 = string(m[1]);
                string sup_str2 = dir_entry.path().filename().string();

                if(sup_str1.find(sup_str2) != string::npos && sup_str1.at(0) == sup_str2.at(0)){
                   
                    path p = in_file.parent_path() / sup_str1;

                    ifstream sup_f(p.string());
                    if(sup_f){
                        
                        if(!Preprocess(p, out_file, include_directories))
                            return false;
                        
                        is_found = true;
                        emplaced = true;
                        break;
                    }
                }
            }

            if(!is_found){ 
                string sup_str1 = string(m[1]);
            
                path p = sup_str1;
                for(const path& el : include_directories){
                    path d = el / p;
                
                    ifstream sup_f(d.string());

                    if(sup_f){
                        
                        if(!Preprocess(d, out_file, include_directories))
                            return false;
                        is_found = true;
                        emplaced = true;
                        break;
                    }
                }
                if(!is_found){ 
                    cout <<"unknown include file "s << string(m[1]) << " at file "s << in_file.string()<< " at line "s << string_number << endl;
                    return false;
                }
            }
        } 

        if(std::regex_search(s, m, incl_reg2)){
            bool is_found = false;

            string sup_str1 = string(m[1]);
            
            path p = sup_str1;
            for(const path& el : include_directories){
                path d = el / p;
                
                ifstream sup_f(d.string());

                if(sup_f){
                    
                    if(!Preprocess(d, out_file, include_directories))
                            return false;
                    
                    is_found = true;
                    emplaced = true;
                    break;
                }
                
            }
            if(!is_found){
                cout <<"unknown include file "s << string(m[1]) << " at file "s << in_file.string()<< " at line "s << string_number << endl;
                return false;
            }
        }
        if(!emplaced) file_ << s << endl;
        string_number++;
    }
    return true;
}

string GetFileContents(string file) {
    ifstream stream(file);

    // конструируем string по двум итераторам
    return {(istreambuf_iterator<char>(stream)), istreambuf_iterator<char>()};
}

void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
                "#include \"dir1/b.h\"\n"
                "// text between b.h and c.h\n"
                "#include \"dir1/d.h\"\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"
                "#   include<dummy.txt>\n"
                "}\n"sv;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
                "#include \"subdir/c.h\"\n"
                "// text from b.h after include"sv;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
                "#include <std1.h>\n"
                "// text from c.h after include\n"sv;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
                "#include \"lib/std2.h\"\n"
                "// text from d.h after include\n"sv;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"sv;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"sv;
    }

    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
                                  {"sources"_p / "include1"_p,"sources"_p / "include2"_p})));

    ostringstream test_out;
    test_out << "// this comment before include\n"
                "// text from b.h before include\n"
                "// text from c.h before include\n"
                "// std1\n"
                "// text from c.h after include\n"
                "// text from b.h after include\n"
                "// text between b.h and c.h\n"
                "// text from d.h before include\n"
                "// std2\n"
                "// text from d.h after include\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"sv;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    Test();
}