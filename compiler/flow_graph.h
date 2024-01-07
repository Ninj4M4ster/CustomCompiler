#ifndef CUSTOMCOMPILER_COMPILER_FLOW_GRAPH_H_
#define CUSTOMCOMPILER_COMPILER_FLOW_GRAPH_H_

#include "data.h"

class GraphNode {
 public:
  std::vector<Command*> commands;
  std::shared_ptr<Condition> cond = nullptr;
  std::shared_ptr<GraphNode> left_node = nullptr;
  std::shared_ptr<GraphNode> right_node = nullptr;

  // start line of current node
  long long int start_line_ = -1;
  long long int condition_start_line_ = -1;
  long long int node_end_line_ = -1;
  // target of jump command, that should be added after all commands from this, left and right node; it should target start of the code block
  std::shared_ptr<GraphNode> jump_line_target = nullptr;
  // target of jump command, that should be added after all commands from this, left and right node; it should target start of the condition block
  std::shared_ptr<GraphNode> jump_condition_target = nullptr;
  // length of code in this node
  long long int node_length_ = 0;
};

class FlowGraph {
 public:
  void generateFlowGraph(Procedure main, std::vector<Procedure> procedures);

 private:
  std::shared_ptr<GraphNode> generateSingleFlowGraph(Procedure proc);
  void process_commands(std::vector<Command*> comms, std::shared_ptr<GraphNode> curr_node);
  std::shared_ptr<GraphNode> start_node;
  std::vector<std::shared_ptr<GraphNode>> procedures_start_nodes_;
};

#endif //CUSTOMCOMPILER_COMPILER_FLOW_GRAPH_H_
