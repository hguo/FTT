#ifndef _FTK_UNION_FIND_H
#define _FTK_UNION_FIND_H

#include <vector>
#include <map>
#include <set>
#include <iostream>

// Reference 
  // Paper: "Worst-case Analysis of Set Union Algorithms"
  // Online slides: https://www.cs.princeton.edu/~rs/AlgsDS07/01UnionFind.pdf

// Add the sparse representation by using Hash map/table 

namespace ftk {
template <class IdType=std::string>
struct union_find
{
  union_find() {
    
  }

  // Initialization
  // Add and initialize elements
  void add(IdType i) {
    eles.insert(i); 
    // id2parent[i] = i; 
    id2parent.insert(std::make_pair (i, i)); 
    sz[i] = 1;
  }

  // Operations
  
  // Union by size
  bool unite(IdType i, IdType j) {
    if(!has(i) || !has(j)) {
      return false ;
    }

    i = find(i);
    j = find(j);

    if (sz[i] < sz[j]) {
      // id2parent[i] = j; 
      id2parent.find(i)->second = j; 
      sz[j] += sz[i];
    } else {
      // id2parent[j] = i;
      id2parent.find(j)->second = i; 
      sz[i] += sz[j];
    }

    return true; 
  }

  // Queries

  bool has(IdType i) {
    return eles.find(i) != eles.end(); 
  }

  IdType parent(IdType i) {
    if(!has(i)) {
      return i; 
    }

    // return id2parent[i]; 
    return id2parent.find(i)->second; 
  }

  // Find the root of an element. 
    // If the element is not in the data structure, return the element itself. 
    // Path compression by path halving method
  IdType find(IdType i) {
    if(!has(i)) {
      return i; 
    }

    IdType& parent_i = id2parent.find(i)->second; 
    while (i != parent_i) {
      // Set parent of i to its grandparent
      id2parent.find(i)->second = id2parent.find(parent_i)->second; 

      i = id2parent.find(i)->second;
      parent_i = id2parent.find(i)->second; 
    }

    return i; 
  }

  bool is_root(IdType i) {
    if(!has(i)) {
      return false; 
    }

    return i == id2parent[i]; 
  }

  bool same_set(IdType i, IdType j) {
    return find(i) == find(j);
  }

  std::vector<std::set<IdType>> get_sets() {
    std::map<IdType, std::set<IdType>> root2set; 
    for(auto ite = eles.begin(); ite != eles.end(); ++ite) {
      IdType root = find(*ite); 

      root2set[root].insert(*ite);
    }

    std::vector<std::set<IdType>> results; 
    for(auto ite = root2set.begin(); ite != root2set.end(); ++ite) {
      // if(is_root(*ite))
      results.push_back(ite->second); 
    }

    return results; 
  }

public:
  std::set<IdType> eles; 

private:
  // Use HashMap to support sparse union-find
  std::map<IdType, IdType> id2parent;
  std::map<IdType, size_t> sz;
};

}

#endif
