/**
 * @file src/demangler_llvm/borland_ast.cpp
 * @brief Implementation of syntactic tree for borland demangler.
 * @copyright (c) 2018 Avast Software, licensed under the MIT license
 */

#include <sstream>
#include <map>

#include "llvm/Demangle/borland_ast.h"
#include "llvm/Demangle/borland_ast_types.h"
#include "llvm/Demangle/context.h"

namespace retdec {
namespace demangler {
namespace borland {

/**
 * @brief Abstract constructor for base node.
 * @param kind Kind of node.
 */
Node::Node(Kind kind, bool has_right_side) :
	_kind(kind), _has_right(has_right_side) {}

/**
 * @brief Prints left side of node.
 * @param s output stream
 */
void Node::print(std::ostream &s) const
{
	printLeft(s);
	if (_has_right) {
		printRight(s);
	}
}

/**
 * @return String representation of node.
 */
std::string Node::str() const
{
	std::stringstream ss;
	print(ss);
	return ss.str();
}

/**
 * @return Kind of node.
 */
Node::Kind Node::kind() const
{
	return _kind;
}

/**
 * @brief Some nodes need special trailing characters.
 * @param s output stream.
 */
void Node::printRight(std::ostream &s) const {}

bool Node::hasRight()
{
	return _has_right;
}

/**
 * @brief Private function node constructor. Use create().
 * @param call_conv Pointer to calling convention.
 * @param name Pointer to Name or NestedName node.
 * @param params Pointer to parameters.
 */
FunctionNode::FunctionNode(
	std::shared_ptr<retdec::demangler::borland::Node> name,
	std::shared_ptr<FunctionTypeNode> funcType) :
	Node(Kind::KFunction, false),
	_name(std::move(name)),
	_funcNode(funcType) {}

/**
 * @brief Creates shared pointer to function node.
 * @param call_conv Pointer to calling convention node.
 * @param name Pointer to Name or NestedName node.
 * @param params Pointer to parameters.
 * @return Unique pointer to constructed FunctionNode.
 */
std::shared_ptr<FunctionNode> FunctionNode::create(
	std::shared_ptr<retdec::demangler::borland::Node> name,
	std::shared_ptr<FunctionTypeNode> funcType)
{
	return std::shared_ptr<FunctionNode>(new FunctionNode(name, funcType));
}

/**
 * @brief Prints text representation of function.
 * @param s Output stream.
 */
void FunctionNode::printLeft(std::ostream &s) const
{
	_funcNode->printLeft(s);
	_name->print(s);
	_funcNode->printRight(s);
}

TemplateNode::TemplateNode(
	std::shared_ptr<retdec::demangler::borland::Node> name,
	std::shared_ptr<retdec::demangler::borland::Node> params) :
	Node(Kind::KTemplateNode), _name(name), _params(params) {}

std::shared_ptr<TemplateNode> TemplateNode::create(
	std::shared_ptr<retdec::demangler::borland::Node> name,
	std::shared_ptr<retdec::demangler::borland::Node> params)
{
	return std::shared_ptr<TemplateNode>(new TemplateNode(name, params));
}

void TemplateNode::printLeft(std::ostream &s) const
{
	_name->print(s);
	s << "<";
	if (_params) {
		_params->print(s);
	}
	s << ">";
}

/**
 * @brief Constructor for NameNode
 * @param name std::string representation of name.
 */
NameNode::NameNode(const std::string &name) : Node(NameNode::Kind::KName, false), _name(name) {}

/**
 * @param name std::string representation of name.
 * @return Unique pointer to new NameNode
 */
std::shared_ptr<NameNode> NameNode::create(Context &context, const std::string &name)
{
	auto type = context.getName(name);
	if (type) {
		return type;
	}

	auto newName = std::shared_ptr<NameNode>(new NameNode(name));
	context.addName(newName);
	return newName;
}

/**
 * @brief Prints left side of node represention.
 * @param s output stream
 */
void NameNode::printLeft(std::ostream &s) const
{
	s << _name;
}

/**
 * NestedName constructor.
 * @param super Higher level node.
 * @param name Lower level node.
 */
NestedNameNode::NestedNameNode(
	std::shared_ptr<Node> super, std::shared_ptr<Node> name) :
	Node(NameNode::Kind::KNestedName, false), _super(super), _name(name) {}

/**
 * @param super Higher level node.
 * @param name Lower level node.
 * @return Unique pointer to new nested name node.
 */
std::shared_ptr<NestedNameNode> NestedNameNode::create(
	Context &context, std::shared_ptr<Node> super, std::shared_ptr<Node> name)
{
	auto type = context.getNestedName(super, name);
	if (type) {
		return type;
	}

	auto newName = std::shared_ptr<NestedNameNode>(new NestedNameNode(super, name));
	context.addNestedName(newName);
	return newName;
//	return std::shared_ptr<NestedNameNode>(new NestedNameNode(super, name));
}

/**
 * @brief Prints left side of node represention.
 * @param s output stream
 */
void NestedNameNode::printLeft(std::ostream &s) const
{
	_super->print(s);
	s << std::string{"::"};
	_name->print(s);
}

std::shared_ptr<Node> NestedNameNode::super() {
	return _super;
}

std::shared_ptr<Node> NestedNameNode::name() {
	return _name;
}

/**
 * @brief Private constructor for NodeArray. Use create().
 */
NodeArray::NodeArray() : Node(Kind::KNodeArray), _nodes() {}

/**
 * @brief Creates shared pointer to new NodeArray object.
 * @return Pointer to empty ArrayNode.
 */
std::shared_ptr<NodeArray> NodeArray::create()
{
	return std::shared_ptr<NodeArray>(new NodeArray());
}

/**
 * @brief Appends new node to array.
 * @param node Node to be added.
 */
void NodeArray::addNode(std::shared_ptr<retdec::demangler::borland::Node> node)
{
	_nodes.push_back(node);
}

bool NodeArray::empty() const
{
	return _nodes.empty();
}

size_t NodeArray::size()
{
	return _nodes.size();
}

/**
 * @brief Prints text representaion of array.
 * @param s Output stream.
 */
void NodeArray::printLeft(std::ostream &s) const
{
	if (!_nodes.empty()) {
		/* print first */
		auto current = _nodes.begin();
		(*current)->print(s);

		/* print others */
		while (++current != _nodes.end()) {
			s << ", ";
			(*current)->print(s);
		}
	}
}

std::shared_ptr<Node> NodeArray::get(unsigned i) const
{
	return i < _nodes.size() ? _nodes.at(i): nullptr;
}

Qualifiers::Qualifiers(bool isVolatile, bool isConst) :
	_isVolatile(isVolatile), _isConst(isConst) {}

bool Qualifiers::isVolatile() const
{
	return _isVolatile;
}

bool Qualifiers::isConst() const
{
	return _isConst;
}

void Qualifiers::printSpaceL(std::ostream &s) const
{
	if (_isVolatile) {
		s << " volatile";
	}
	if (_isConst) {
		s << " const";
	}
}

void Qualifiers::printSpaceR(std::ostream &s) const
{
	if (_isVolatile) {
		s << "volatile ";
	}
	if (_isConst) {
		s << "const ";
	}
}

ConversionOperatorNode::ConversionOperatorNode(std::shared_ptr<retdec::demangler::borland::Node> type) :
	Node(Kind::KConversionOperator), _type(type) {}

std::shared_ptr<ConversionOperatorNode> ConversionOperatorNode::create(Context &context, std::shared_ptr<Node> type)
{
	return std::shared_ptr<ConversionOperatorNode>(new ConversionOperatorNode(type));
}

void ConversionOperatorNode::printLeft(std::ostream &s) const
{
	s << "operator ";
	_type->print(s);
}

}    // borland
}    // demangler
}    // retdec
