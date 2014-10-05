#include "simplification.h"
#include "ctfunc.h"

#include <stack>
#include <algorithm>
#include <vector>
#include <tbb/parallel_for.h>

bool marked_for_removal(ctBranch* b) {
  if(b == NULL) return true;
  FeatureSet* fs = (FeatureSet*) b->data;
  return fs->remove;
  return fs == NULL? false : fs->remove;
}

bool branch_is_leaf(ctBranch* b)
{
  if(b == NULL || b->children.head == NULL) return true;

  bool will_remove = true;
  for(ctBranch* c = b->children.head; c != NULL && will_remove == true; c = c->nextChild)
    will_remove = marked_for_removal(c);

  return will_remove;
}

void mark_for_removal(ctBranch* root_branch, ctBranch** branch_map, size_t data_size, double (*importance_measure)(ctBranch*), double threshold)
{
  if(root_branch == NULL) return;
  FeatureSet* branch_data = (FeatureSet*) root_branch->data;

  for(ctBranch* c = root_branch->children.head; c != NULL; c = c->nextChild)
    mark_for_removal(c, branch_map, data_size, importance_measure, threshold);

  if(branch_is_leaf(root_branch)) {
    branch_data->remove = importance_measure(root_branch) > threshold ? false : true;
    //importance_measure(root_branch) > threshold ? branch_data->remove = false : branch_data->remove = true;
    //TODO: Associate the vertices with the parent branch
    for(size_t i = 0; i < data_size; i++) {
      if(branch_map[i] == root_branch && root_branch->parent != NULL) {
        branch_map[i] = root_branch->parent;
      }
    }
  }
}

DEPRECATED void simplify_tree_dfs(ctBranch* root_branch, ctBranch** branch_map, size_t data_size, double (*importance_measure)(ctBranch*), double threshold)
{
  mark_for_removal(root_branch, branch_map, data_size, importance_measure, threshold);
  //    remove_marked_branches(root_branch, branch_map, data, ctx);
  //    if(root_branch == NULL) return;
  //    FeatureSet* branch_data = (FeatureSet*) root_branch->data;

  //    for(ctBranch* c = root_branch->children.head; c != NULL; c = c->nextChild)
  //        simplify_tree_dfs(c, branch_map, data, ctx, importance_measure, threshold);

  //    if(/*root_branch->children.head == NULL*/branch_is_leaf(root_branch)) {
  //        double arc_importance = importance_measure(root_branch);
  //        if(arc_importance < threshold) {
  //            branch_data->remove = true;
  //            //remove_branch(root_branch, branch_map, data, ctx);
  //        } else {
  //            std::cout << "Volume: " << branch_data->v << "\tHypervolume: " << branch_data->hv << "\tPersistance: " << branch_data->p << std::endl;
  //        }
  //        return;
  //    }
}

DEPRECATED void simplify_from_branchmap(ctBranch** branch_map, size_t map_size, double(*importance_measure)(ctBranch*), double threshold)
{
  bool changed = false;

  do {
    changed = false;

    for(size_t i = 0; i < map_size; i++) {
      FeatureSet* branch_data = (FeatureSet*) branch_map[i]->data;

      if(branch_is_leaf(branch_map[i]) && branch_data->remove == false) {

        if(importance_measure(branch_map[i]) > threshold) {
          branch_data->remove = false;
        } else {
          branch_data->remove = true;
          branch_map[i] = branch_map[i]->parent;
          changed = true;
        }

      }
    }

  } while(changed == true);
}

static std::vector<ctBranch*> queue_leaves(ctBranch* root_branch)
{
  std::vector<ctBranch*> leaves;
  if(root_branch == NULL)
    return leaves;

  std::stack<ctBranch*> stack;
  stack.push(root_branch);

  do {
    ctBranch* curr_branch = stack.top();

    stack.pop();

    if(curr_branch->children.head == NULL)
      leaves.push_back(curr_branch);

    for(ctBranch* c = curr_branch->children.head; c != NULL; c = c->nextChild)
      stack.push(c);

  } while(!stack.empty());

  return leaves;
}

// class PointToParent
// {
//   ctBranch** branch_map;
//   ctBranch* to_remove;

// public:

//   PointToParent(ctBranch** a, ctBranch* to_rem) : branch_map(a), to_remove(to_rem)
//   {}

//   void operator()(const tbb::blocked_range<size_t>& range) const {

//     FeatureSet* rem_data = (FeatureSet*) to_remove->data;

//     for(int i = range.begin(); i != range.end(); i++) {
//       FeatureSet* bmap_data = (FeatureSet*) branch_map[i]->data;
//       if(bmap_data->label == rem_data->label) {
//         branch_map[i] = to_remove->parent;
//       }
//     }
//   }
// };

static void point_to_parent(ctBranch* to_remove, ctBranch** branch_map, size_t map_size)
{
  FeatureSet* rem_data = (FeatureSet*) to_remove->data;
  FeatureSet* par_data = (FeatureSet*) to_remove->parent->data;

  std::vector<size_t> new_vertices(rem_data->vertices.size() + par_data->vertices.size());
  std::merge(rem_data->vertices.begin(), rem_data->vertices.end(),
  	     par_data->vertices.begin(), par_data->vertices.end(),
  	     new_vertices.begin());

  par_data->vertices = new_vertices;
  // tbb::parallel_for(tbb::blocked_range<size_t>(0, map_size), PointToParent(branch_map, to_remove));
}

void topSimplifyTree(ctContext* ctx, ctBranch* root_branch, ctBranch** branch_map, size_t map_size, double(*importance_cb)(ctBranch*), double thresh)
{
  using namespace std;

  if(count_branches(root_branch) == 1)
    return;

  bool changed = false;

  do {
    vector<ctBranch*> leaves = queue_leaves(root_branch);
    changed = false;

    for(size_t i = 0; i < leaves.size(); i++) {
      if(leaves[i] == NULL)
        continue;

      if(importance_cb(leaves[i]) < thresh) {
        point_to_parent(leaves[i], branch_map, map_size);
        ctBranch_delete(leaves[i], ctx);
        changed = true;
      }
    }

    leaves.clear();

  } while(changed);
}

class ReduceToSaddle
{
  ctBranch** branch_map;
  ctBranch* to_remove;
  top::Dataset& top_data;

public:

  ReduceToSaddle(ctBranch** a, ctBranch* to_rem, top::Dataset& t) : branch_map(a), to_remove(to_rem), top_data(t)
  {}

  void operator()(const tbb::blocked_range<size_t>& range) const {

    FeatureSet* rem_data = (FeatureSet*) to_remove->data;

    for(int i = range.begin(); i != range.end(); i++) {
      FeatureSet* bmap_data = (FeatureSet*) branch_map[i]->data;
      if(bmap_data->label == rem_data->label) {
        size_t saddle = top_data.data->Get(to_remove->saddle);
        top_data.data->Set(i, saddle);
      }
    }
  }
};

static void reduce_to_saddle(ctBranch* to_remove, ctBranch** branch_map, top::Dataset& top_data) 
{
  FeatureSet* rem_data = (FeatureSet*) to_remove->data;

  tbb::parallel_for(tbb::blocked_range<size_t>(0, top_data.size), ReduceToSaddle(branch_map, to_remove, top_data));
}

void topSimplifyTreeZhou(ctContext* ctx, ctBranch* root_branch, ctBranch** branch_map, top::Dataset& top_data, double(*importance_cb)(ctBranch*), double thresh)
{
  
  using namespace std;

  if(count_branches(root_branch) == 1)
    return;

  bool changed = false;

  do {
    vector<ctBranch*> leaves = queue_leaves(root_branch);
    changed = false;

    for(size_t i = 0; i < leaves.size(); i++) {
      if(leaves[i] == NULL)
        continue;

      if(importance_cb(leaves[i]) < thresh) {
        reduce_to_saddle(leaves[i], branch_map, top_data);
        ctBranch_delete(leaves[i], ctx);
        changed = true;

      }
    }

    leaves.clear();

  } while(changed);
}
