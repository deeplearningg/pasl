/* COPYRIGHT (c) 2014 Umut Acar, Arthur Chargueraud, and Michael
 * Rainey
 * All rights reserved.
 *
 * \file check.cpp
 * \brief Unit testing driver
 *
 */

#include <math.h>

#include "benchmark.hpp"
#include "hash.hpp"
#include "dup.hpp"
#include "string.hpp"
#include "sort.hpp"
#include "graph.hpp"

/***********************************************************************/

/*---------------------------------------------------------------------*/
/* Quickcheck library initialization */

namespace quickcheck {
  
  void generate(size_t target_nb_edges, edgelist& edges);
  
  std::ostream& operator<<(std::ostream& out, const edgelist& edges) {
    return output_directed_dot(out, edges);
  }
  
}

#include "quickcheck.hh" // needs to appear at end of include list

/*---------------------------------------------------------------------*/

long nb_tests;

template <class Property>
void checkit(std::string msg) {
  quickcheck::check<Property>(msg.c_str(), nb_tests);
}

bool same_array(const_array_ref xs, const_array_ref ys) {
  if (xs.size() != ys.size())
    return false;
  for (long i = 0; i < xs.size(); i++)
    if (xs[i] != ys[i])
      return false;
  return true;
}

array array_of_vector(const std::vector<value_type>& vec) {
  return tabulate([&] (long i) { return vec[i]; }, vec.size());
}

/*---------------------------------------------------------------------*/
/* Unit tests for sorting algorithms */

template <class Trusted_sort_fct, class Untrusted_sort_fct>
class sort_correct : public quickcheck::Property<std::vector<value_type>> {
public:
  
  Trusted_sort_fct trusted_sort;
  Untrusted_sort_fct untrusted_sort;
  
  bool holdsFor(const std::vector<value_type>& vec) {
    array xs = array_of_vector(vec);
    return same_array(trusted_sort(xs), untrusted_sort(xs));
  }
  
};

void check_sort() {
  pasl::util::cmdline::argmap_dispatch c;
  class trusted_fct {
  public:
    array operator()(const_array_ref xs) {
      return seqsort(xs);
    }
  };
  c.add("mergesort", [&] {
    class untrusted_fct {
    public:
      array operator()(const_array_ref xs) {
        return mergesort(xs);
      }
    };
    using property_type = sort_correct<trusted_fct, untrusted_fct>;
    checkit<property_type>("checking mergesort");
  });
  c.add("quicksort", [&] {
    class untrusted_fct {
    public:
      array operator()(const_array_ref xs) {
        return quicksort(xs);
      }
    };
    using property_type = sort_correct<trusted_fct, untrusted_fct>;
    checkit<property_type>("checking quicksort");
  });
  c.find_by_arg("algo")();
}

/*---------------------------------------------------------------------*/
/* Unit tests for graph algorithms */

edgelist gen_random_edgelist(unsigned long dim, unsigned long degree, unsigned long nb_rows) {
  edgelist edges;
  long nb_nonzeros = degree * nb_rows;
  edges.resize(nb_nonzeros);
  for (long k = 0; k < nb_nonzeros; k++) {
    unsigned long i = k / degree;
    unsigned long j;
    if (dim==0) {
      unsigned long h = k;
      do {
        j = ((h = hash_unsigned(h)) % nb_rows);
      } while (j == i);
    } else {
      unsigned long pow = dim+2;
      unsigned long h = k;
      do {
        while ((((h = hash_unsigned(h)) % 1000003) < 500001)) pow += dim;
        j = (i + ((h = hash_unsigned(h)) % (((unsigned long) 1) << pow))) % nb_rows;
      } while (j == i);
    }
    edges[k].first = i;
    edges[k].second = j;
  }
  return edges;
}

edgelist gen_random_edgelist(long target_nb_edges) {
  long dim = 10;
  long degree = 8;
  long nb_rows = std::min(degree, target_nb_edges);
  return gen_random_edgelist(dim, degree, nb_rows);
}

adjlist gen_random_adjlist(long target_nb_edges) {
  return adjlist(gen_random_edgelist(target_nb_edges));
}

edgelist gen_balanced_tree_edgelist(long branching_factor, long height) {
  std::vector<vtxid_type> prev;
  std::vector<vtxid_type> next;
  edgelist edges;
  prev.push_back(vtxid_type(0));
  vtxid_type fresh = 1;
  for (vtxid_type level = 0; level < height; level++) {
    for (auto it = prev.begin(); it != prev.end(); it++) {
      vtxid_type v = *it;
      for (vtxid_type n = 0; n < branching_factor; n++) {
        vtxid_type child = fresh;
        fresh++;
        next.push_back(child);
        edges.push_back(mk_edge(v, child));
      }
    }
    prev.clear();
    prev.swap(next);
  }
  return edges;
}

edgelist gen_balanced_tree_edgelist(long target_nb_edges) {
  long branching_factor = 2;
  long height = log2_up(target_nb_edges) - 1;
  return gen_balanced_tree_edgelist(branching_factor, height);
}

adjlist gen_balanced_tree_adjlist(long target_nb_edges) {
  return adjlist(gen_balanced_tree_edgelist(target_nb_edges));
}

edgelist gen_cube_grid_edgelist(long nb_on_side, long) {
  edgelist edges;
  long dn = nb_on_side;
  long nn = dn*dn*dn;
  long nb_edges = 3*nn;
  edges.resize(nb_edges);
  auto loc3d = [&](vtxid_type x, vtxid_type y, vtxid_type z) {
    return ((x + dn) % dn)*dn*dn + ((y + dn) % dn)*dn + (z + dn) % dn;
  };
  for (long i = 0; i < dn; i++) {
    for (vtxid_type j = 0; j < dn; j++) {
      for (vtxid_type k = 0; k < dn; k++) {
        vtxid_type l = loc3d(i,j,k);
        edges[3*l] =   mk_edge(l,loc3d(i+1,j,k));
        edges[3*l+1] = mk_edge(l,loc3d(i,j+1,k));
        edges[3*l+2] = mk_edge(l,loc3d(i,j,k+1));
      }
    }
  }
  return edges;
}

edgelist gen_cube_grid_edgelist(long target_nb_edges) {
  long nb_on_side = long(pow(double(target_nb_edges / 3.0), 1.0/3.0));
  return gen_cube_grid_edgelist(nb_on_side, 0l);
}

adjlist gen_cube_grid_adjlist(long target_nb_edges) {
  return adjlist(gen_cube_grid_edgelist(target_nb_edges));
}

namespace quickcheck {
  void generate(size_t target_nb_edges, edgelist& edges) {
    std::vector<std::function<void ()>> gens;
    gens.push_back([&] {
      edges = gen_random_edgelist(target_nb_edges);
    });
    gens.push_back([&] {
      edges = gen_cube_grid_edgelist(target_nb_edges);
    });
    gens.push_back([&] {
      edges = gen_balanced_tree_edgelist(target_nb_edges);
    });
    long i = random() % gens.size();
    gens[i]();
  }
}

template <class Trusted_bfs_fct, class Untrusted_bfs_fct>
class bfs_correct : public quickcheck::Property<edgelist> {
public:
  
  Trusted_bfs_fct trusted_bfs;
  Untrusted_bfs_fct untrusted_bfs;
  
  bool holdsFor(const edgelist& edges) {
    adjlist graph = adjlist(edges);
    return same_array(trusted_bfs(graph, 0), untrusted_bfs(graph, 0));
  }
  
};

void check_graph() {
  pasl::util::cmdline::argmap_dispatch c;
  class trusted_fct {
  public:
    array operator()(const adjlist& graph, vtxid_type source) {
      return bfs_seq(graph, source);
    }
  };
  c.add("bfs", [&] {
    class untrusted_fct {
    public:
      array operator()(const adjlist& graph, vtxid_type source) {
        atomic_value_ptr p = bfs_par(graph, source);
        long nb_vertices = graph.get_nb_vertices();
        return tabulate([&] (long i) { return p[i].load(); }, nb_vertices);
      }
    };
    using property_type = bfs_correct<trusted_fct, untrusted_fct>;
    checkit<property_type>("checking bfs");
  });
  c.find_by_arg("algo")();
}

/*---------------------------------------------------------------------*/
/* PASL Driver */

void check() {
  nb_tests = pasl::util::cmdline::parse_or_default_long("nb_tests", 500);
  pasl::util::cmdline::argmap_dispatch c;
  c.add("sort", std::bind(check_sort));
  c.add("graph", std::bind(check_graph));
  c.find_by_arg("check")();
}

int main(int argc, char** argv) {
  
  auto init = [&] {
    
  };
  auto run = [&] (bool) {
    check();
  };
  auto output = [&] {
  };
  auto destroy = [&] {
  };
  pasl::sched::launch(argc, argv, init, run, output, destroy);
}

/***********************************************************************/
