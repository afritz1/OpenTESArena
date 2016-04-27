#ifndef BINARY_TREE_H
#define BINARY_TREE_H

#include <memory>

template <typename T>
class BinaryTree
{	
private:
	class Node
	{
	private:
		T data;
		std::unique_ptr<Node> left, right;
	public:
		Node(const T &data)
		{
			this->data = data;
			this->left = nullptr;
			this->right = nullptr;
		}

		Node(const Node &node)
		{
			this->data = T(node.getData());
			this->left = (node.getLeftChild() != nullptr) ?
				std::unique_ptr<Node>(new Node(*node.getLeftChild())) : nullptr;
			this->right = (node.getRightChild() != nullptr) ?
				std::unique_ptr<Node>(new Node(*node.getRightChild())) : nullptr;
		}

		~Node() { }

		const T &getData() const
		{
			return this->data;
		}

		Node *getLeftChild() const
		{
			return this->left.get();
		}

		Node *getRightChild() const
		{
			return this->right.get();
		}

		void setLeftChild(const T &data)
		{
			if (this->left != nullptr)
			{
				this->left->data = data;
			}
			else
			{
				this->left = std::unique_ptr<Node>(new Node(data));
			}
		}

		void setRightChild(const T &data)
		{
			if (this->right != nullptr)
			{
				this->right->data = data;
			}
			else
			{
				this->right = std::unique_ptr<Node>(new Node(data));
			}
		}
	};

	std::unique_ptr<Node> root;

	Node *get(const T &data, Node *leaf) const
	{
		if (leaf != nullptr)
		{
			if (data == leaf->getData())
			{
				return leaf;
			}
			else if (data < leaf->getData())
			{
				return this->get(data, leaf->getLeftChild());
			}
			else
			{
				return this->get(data, leaf->getRightChild());
			}
		}
		else
		{
			return nullptr;
		}
	}

	void insert(const T &data, Node *leaf)
	{
		if (data < leaf->getData())
		{
			if (leaf->getLeftChild() != nullptr)
			{
				this->insert(data, leaf->getLeftChild());
			}
			else
			{
				leaf->setLeftChild(data);
			}
		}
		else if (data >= leaf->getData())
		{
			if (leaf->getRightChild() != nullptr)
			{
				this->insert(data, leaf->getRightChild());
			}
			else
			{
				leaf->setRightChild(data);
			}
		}
	}
public:
	BinaryTree()
	{
		this->root = nullptr;
	}

	BinaryTree(const BinaryTree<T> &tree)
	{
		this->root = (tree.root != nullptr) ?
			std::unique_ptr<Node>(new Node(*tree.root)) : nullptr;
	}

	~BinaryTree() { }

	bool contains(const T &data) const
	{
		return this->get(data, this->root.get()) != nullptr;
	}

	void insert(const T &data)
	{
		if (this->root != nullptr)
		{
			this->insert(data, this->root.get());
		}
		else
		{
			this->root = std::unique_ptr<Node>(new Node(data));
		}
	}
};

#endif
