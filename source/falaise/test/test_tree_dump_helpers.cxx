#include <bayeux/datatools/i_tree_dump.h>
#include <bayeux/datatools/properties.h>

// Print cases
// 1. An atom: type that supplies an operator<< ?
// 2. A Sequence of atoms
//    - can be printed as a list "name : [a, b , ... , z]"
//    - or as a tree
// 3. A tree_dumpable (or "table")
// 4. Sequence of tree_dumpables (or "sequence<table>")

// Can handle
// 1. types that have an operator<<
// 2. types that have are derived from tree_dump
// 3. Ranges of either
//
// Any streaming always has three parameters
// - the ostream
// - current indent
// - boolean, true if last item to be output in the current branch


template <typename T>
using is_table = std::is_base_of<datatools::i_tree_dumpable, T>;



template <typename T, typename std::enable_if<!is_table<T>::value, T>::type* = nullptr>
void tree_dump(std::ostream& os, const std::string& /*indent*/, const T& value) {
  os << value;
}

template <typename T, typename std::enable_if<is_table<T>::value, T>::type* = nullptr>
void tree_dump(std::ostream& os, const std::string& indent, const T& value) {
  std::ostringstream indent_oss;
  indent_oss << indent << datatools::i_tree_dumpable::skip_tag; 
  value.tree_dump(os,"",indent_oss.str());
}


// Incase we have maps...
template <typename First, typename Second>
void tree_dump(std::ostream& os, const std::string& /*indent*/, const std::pair<First, Second>& value) {
  os << "(" << value.first << ", " << value.second << ")";
}

template <typename BidirIt>
void tree_dump(std::ostream& os, const std::string& indent, BidirIt first, BidirIt last) {
  // May need to tag-dispatch here
  // Three possibilities
  // 1. Have sequence of Atoms
  //    1a. Print as a table
  // 2. Have sequence of Tables
  //
  // So two parameters 
  // - Check on is_table<BidirIt::value_type>
  // - Choice to print plain sequence as table
  //
  // Also affects downstream functions in whether to include newline or not...
  // So may need to have as_sequence or as_table functions.
  // Defaults can be "as_sequence" for value_type != tree, "as_tree" otherwise

  auto penultimate = --last;
  while (first != penultimate) {
    ::tree_dump(os, indent, *first);
    ++first;
  }
  ::tree_dump(os, indent, *last);
}

// Review how boost::program_options does this, very similar idea...
// 
// options_description desc{"options"};
// desc.add_options()
//   (...)
//   ...
//   (...); 
struct tree_dumper {
  std::ostream& os;
  const std::string indent;

  template <typename T>
  tree_dumper& operator()(const std::string& key, const T& value) {
    // or last_tag for last
    os << indent << datatools::i_tree_dumpable::tag << key << " : ";
    ::tree_dump(os, indent, value);
    os << std::endl;
    return *this;
  }

  template <typename BidirIt>
  tree_dumper& operator()(const std::string& key, BidirIt first, BidirIt last) {
    // Or last_tag for last
    os << indent << datatools::i_tree_dumpable::tag << key << " : ";
    ::tree_dump(os, indent, first, last);
    return *this;
  }
};

/////////////////////////////////


// From C++14
//template <typename T>
//inline constexpr bool is_table_v = is_table<T>::value;


/* free function might work, but have three parameters to pass to tree_dump...
std::ostream& operator<<(std::ostream& os, const datatools::i_tree_dumpable& o) {
  o.tree_dump(os, title, indent, islast);
  return os;
}
*/





void print_title(std::ostream& os, const std::string& s, const std::string& indent) {
  os << indent << "+-- " << (s == "" ? "<notitle>" : s) << std::endl;
}


template <typename Atom>
void as_atom(std::ostream& os, const std::string& indent, const std::string& name, const Atom& value) {
  os << indent << datatools::i_tree_dumpable::tag << name << " : " << value << std::endl;
}

template <typename Atom>
void as_atom_last(std::ostream& os, const std::string& indent, const std::string& name, const Atom& value) {
  os << indent << datatools::i_tree_dumpable::last_tag << name << " : " << value << std::endl;
}

template <typename Sequence>
void as_sequence(std::ostream& os, const std::string& indent, const std::string& name, const Sequence& value) {
  os << indent << datatools::i_tree_dumpable::tag << name << " : ";

  auto iter = value.begin();
  auto last = --value.end();

  os << "[";
  while (iter != last) {
    os << *iter
       << ", ";
    ++iter;
  }
  os << *last << "]" << std::endl; 
}

template <typename Sequence>
void as_sequence_last(std::ostream& os, const std::string& indent, const std::string& name, const Sequence& value) {
  os << indent << datatools::i_tree_dumpable::last_tag << name << " : ";

  auto iter = value.begin();
  auto last = --value.end();

  os << "[";
  while (iter != last) {
    os << *iter
       << ", ";
    ++iter;
  }
  os << *last << "]" << std::endl; 
}


template <typename Sequence>
void as_tree(std::ostream& os, const std::string& indent, const std::string& name, const Sequence& value) {
  
  os << indent << datatools::i_tree_dumpable::tag << name << " : " << std::endl;

  auto iter = value.begin();
  auto last = --value.end();

  while (iter != last) {
    os << indent 
       << datatools::i_tree_dumpable::skip_tag 
       << datatools::i_tree_dumpable::tag 
       << *iter
       << std::endl;
       ++iter;
  }
  os << indent 
     << datatools::i_tree_dumpable::skip_tag 
     << datatools::i_tree_dumpable::last_tag
     << *last
     << std::endl;
}

template <typename Sequence>
void as_tree_last(std::ostream& os, const std::string& indent, const std::string& name, const Sequence& value) {
  os << indent << datatools::i_tree_dumpable::last_tag << name << " : " << std::endl;

  auto iter = value.begin();
  auto last = --value.end();

  while (iter != last) {
    os << indent 
       << datatools::i_tree_dumpable::last_skip_tag
       << datatools::i_tree_dumpable::tag 
       << *iter
       << std::endl;
       ++iter;
  }
  os << indent 
     << datatools::i_tree_dumpable::last_skip_tag 
     << datatools::i_tree_dumpable::last_tag
     << *last
     << std::endl;
   
}

// NB need some template trickery to allow transparent calls to "as_tree"

void as_tree_(std::ostream& os, const std::string& indent, const std::string& name, const datatools::i_tree_dumpable& value) {
    os << indent << datatools::i_tree_dumpable::tag << name << " : " << std::endl;
    std::ostringstream indent_oss;
    indent_oss << indent << datatools::i_tree_dumpable::skip_tag; 
    value.tree_dump(os,"",indent_oss.str());
}

void as_tree_last_(std::ostream& os, const std::string& indent, const std::string& name, const datatools::i_tree_dumpable& value) {
    os << indent << datatools::i_tree_dumpable::last_tag << name << " : " << std::endl;
    std::ostringstream indent_oss;
    indent_oss << indent << datatools::i_tree_dumpable::last_skip_tag; 
    value.tree_dump(os,"",indent_oss.str(),false);
}



class test : public datatools::i_tree_dumpable {
 public:
  void tree_dump(std::ostream& os = std::clog, const std::string& title = "",
                 const std::string& indent = "", bool /*inherit*/ = false) const override {
    tree_dumper dumper{os, indent};
    dumper("pod", pod)
    ("tree", td)
    ("seq", podVector.begin(), podVector.end())
    ("pset", tp.begin(), tp.end())
    ("map", mp.begin(), mp.end());
    os << indent << title << std::endl;
  }

 private:
  using pset = datatools::properties;
  std::string pod = "hello";
  std::set<std::string> podVector = {"foo", "bar", "baz"};
  pset td{"pset"};
  std::vector<pset> tp{pset{"a"}, pset{"b"}, pset{"c"}};
  std::map<std::string, int> mp{{"hello", 1}, {"world", 2}};
};

int main() {
  std::cout << "-- start --" << std::endl;

  test a;
  a.tree_dump(std::cout, "", "", false);

  a.tree_dump(std::cout, "a title", "", true);
  std::cout << "-- end --" << std::endl;

  std::cout << is_table<decltype(a)>::value << std::endl;
  std::cout << is_table<std::vector<int>>::value << std::endl;

  return 0;
}