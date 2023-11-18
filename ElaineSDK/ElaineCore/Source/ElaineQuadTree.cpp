#include "ElainePrecompiledHeader.h"
#include "ElaineQuadTree.h"

namespace Elaine
{
	QuadTreeNode::QuadTreeNode()
	{

	}

	QuadTreeNode::~QuadTreeNode()
	{
		for (auto node : mSceneNodeSet)
		{
			SAFE_DELETE(node)
		}
		mSceneNodeSet.clear();
		SAFE_DELETE(mRightTopNode)
		SAFE_DELETE(mLeftTopNode)
		SAFE_DELETE(mRightBottomNode)
		SAFE_DELETE(mLeftBottomNode)
		SAFE_DELETE(mParentNode)
	}

	void QuadTreeNode::addSceneNode(SceneNode* node)
	{
		mSceneNodeSet.emplace(node);
	}

	void QuadTreeNode::removeSceneNode(SceneNode* node)
	{
		auto iter = mSceneNodeSet.find(node);
		if (iter == mSceneNodeSet.end())
			return;

		mSceneNodeSet.erase(iter);
	}

	QuadTree::QuadTree()
	{

	}

	QuadTree::~QuadTree()
	{

	}

	QuadTreeNode* QuadTree::createNode()
	{
		return nullptr;
	}

	void QuadTree::addSceneNode(SceneNode* node)
	{
		
	}

	void QuadTree::removeSceneNode(SceneNode* node)
	{

	}
}