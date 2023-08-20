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

void PrintError(const string& current, const string& next, int number_string) {

    cout << "unknown include file "s << current << " at file "s << next << " at line "s << number_string << endl;

}

bool Preprocess_stream(istream& input, ostream& output, const path& file_name, const vector<path>& include_directories) {

    static regex local_match (R"/(\s*#\s*include\s*"([^"]*)"\s*)/");
    static regex global_match (R"/(\s*#\s*include\s*<([^>]*)>\s*)/");
    int string_counter = 0;
    string str;
    smatch matched_local, matched_global;

    while( getline( input, str ) ) {

        string_counter++;

        if (!regex_match(str, matched_local, local_match) && !regex_match(str, matched_global, global_match)) {
            output << str << endl;
        }

        else {
            ifstream in;
            path relative_path;
            path absolute_path;

            if (!matched_local.empty()) {
                relative_path = string(matched_local[1]);
                absolute_path = file_name.parent_path() / relative_path;
                in.open(absolute_path.string(), ios::in);
            }
            else {

                relative_path = string(matched_global[1]);

            }

            for (auto& file: include_directories) {

                if (in.is_open()) {
                        break;
                }

                absolute_path = file / relative_path;
                in.open(absolute_path.string(), ios::in);
            }

            if (!in) {
                PrintError(absolute_path.filename().string(), file_name.string(), string_counter);
                return false;
            }

        Preprocess_stream(in, output, absolute_path.string(), include_directories);
        }
    }

    return true;
}
    
// напишите эту функцию
bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories){

    ifstream file_in(in_file.string(), ios::in);
    if (!file_in) return false;

    ofstream file_out(out_file.string(), ios::out);
    if (!file_out) return false;

    return Preprocess_stream(file_in, file_out, in_file, include_directories);
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