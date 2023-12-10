#pragma once

namespace Elaine
{
#define MAX_DEPTH  8

	class SceneNode;

	class ElaineCoreExport QuadTreeNode
	{
		friend class QuadTree;
	public:
		QuadTreeNode();
		~QuadTreeNode();
		void 		addSceneNode(SceneNode* node);
		void		removeSceneNode(SceneNode* node);
	private:
		std::set<SceneNode*>		mSceneNodeSet;
		QuadTreeNode*				mRightTopNode		= nullptr;
		QuadTreeNode*				mLeftTopNode		= nullptr;
		QuadTreeNode*				mRightBottomNode	= nullptr;
		QuadTreeNode*				mLeftBottomNode		= nullptr;
		QuadTreeNode*				mParentNode			= nullptr;
		int							mdepth				= 0;
	};

	class ElaineCoreExport QuadTree
	{
	public:
		QuadTree();
		~QuadTree();
		QuadTreeNode*			createNode();
		void					addSceneNode(SceneNode* node);
		void					removeSceneNode(SceneNode* node);
		void					findVisibilityObject();
	private:

	private:
		QuadTreeNode*			m_rootNode = nullptr;
		int						m_depth	= 0;
	};
}