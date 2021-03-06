﻿#include "txComponentHeader.h"
#include "txComponentFactory.h"
#include "txComponentFactoryManager.h"
#include "GameLog.h"

std::map<std::string, txComponent*> txComponentOwner::EMPTY_COMPONENT_MAP;

void txComponentOwner::updatePreComponent(const float& elapsedTime)
{
	if (mAllComponentList.empty())
	{
		return;
	}
	int rootComponentCount = mRootComponentList.size();
	// 预更新基础类型组件
	for (int i = 0; i < rootComponentCount; ++i)
	{
		txComponent* component = mRootComponentList[i];
		if (isPreUpdateType(component->getType()))
		{
			if (component != NULL && component->isActive() && !component->isLockOneFrame())
			{
				component->preUpdate(elapsedTime);
			}
		}
	}
	// 更新基础类型组件
	for (int i = 0; i < rootComponentCount; ++i)
	{
		txComponent* component = mRootComponentList[i];
		if (isPreUpdateType(component->getType()))
		{
			if (component != NULL && component->isActive() && !component->isLockOneFrame())
			{
				component->update(elapsedTime);
			}
		}
	}
	// 补充更新基础类型组件
	for (int i = 0; i < rootComponentCount; ++i)
	{
		txComponent* component = mRootComponentList[i];
		if (isPreUpdateType(component->getType()))
		{
			if (component != NULL && component->isActive())
			{
				// 如果组件被锁定了一帧,则不更新,解除锁定
				if (component->isLockOneFrame())
				{
					component->setLockOneFrame(false);
				}
				else
				{
					component->lateUpdate(elapsedTime);
				}
			}
		}
	}
}

void txComponentOwner::updateComponents(const float& elapsedTime)
{
	if (mAllComponentList.empty())
	{
		return;
	}
	int rootComponentCount = mRootComponentList.size();
	// 预更新基础类型组件
	for (int i = 0; i < rootComponentCount; ++i)
	{
		txComponent* component = mRootComponentList[i];
		if (!isPreUpdateType(component->getType()))
		{
			if (component != NULL && component->isActive() && !component->isLockOneFrame())
			{
				component->preUpdate(elapsedTime);
			}
		}
	}
	// 更新基础类型组件
	for (int i = 0; i < rootComponentCount; ++i)
	{
		txComponent* component = mRootComponentList[i];
		if (!isPreUpdateType(component->getType()))
		{
			if (component != NULL && component->isActive() && !component->isLockOneFrame())
			{
				component->update(elapsedTime);
			}
		}
	}
	// 补充更新基础类型组件
	for (int i = 0; i < rootComponentCount; ++i)
	{
		txComponent* component = mRootComponentList[i];
		if (!isPreUpdateType(component->getType()))
		{
			if (component != NULL && component->isActive())
			{
				// 如果组件被锁定了一帧,则不更新,解除锁定
				if (component->isLockOneFrame())
				{
					component->setLockOneFrame(false);
				}
				else
				{
					component->lateUpdate(elapsedTime);
				}
			}
		}
	}
}

void txComponentOwner::notifyComponentAttached(txComponent* component)
{
	if (component == NULL)
	{
		return;
	}
	std::map<std::string, txComponent*>::iterator iter = mAllComponentList.find(component->getName());
	if (iter == mAllComponentList.end())
	{
		addComponentToList(component);
	}
}

bool txComponentOwner::notifyComponentNameChanged(const std::string& oldName, txComponent* component)
{
	// 先查找是否有该名字的组件
	std::map<std::string, txComponent*>::iterator it = mAllComponentList.find(oldName);
	if (it == mAllComponentList.end())
	{
		return false;
	}
	// 再查找改名后会不会重名
	std::map<std::string, txComponent*>::iterator itNew = mAllComponentList.find(component->getName());
	if (itNew != mAllComponentList.end())
	{
		return false;
	}
	removeComponentFromList(component);
	addComponentToList(component);
	return true;
}

txComponent* txComponentOwner::createIndependentComponent(const std::string& name, const std::string& type, const bool& initComponent)
{
	// 查找工厂
	txComponentFactoryBase* factory = mComponentFactoryManager->getFactory(type);
	if (factory == NULL)
	{
		GAME_ERROR("error : can not find component factory! type : %s", type.c_str());
		return NULL;
	}
	// 创建组件并且设置拥有者,然后初始化
	txComponent* component = factory->createComponent(name);
	if (initComponent && component != NULL)
	{
		component->init(NULL);
	}
	return component;
}

txComponent* txComponentOwner::addComponent(const std::string& name, const std::string& type)
{
	// 不能创建重名的组件
	if (mAllComponentList.find(name) != mAllComponentList.end())
	{
		GAME_ERROR("error : there is component named : %s in the list", name.c_str());
		return NULL;
	}
	txComponent* component = createIndependentComponent(name, type, false);
	component->init(this);
	// 将组件加入列表
	addComponentToList(component);
	// 通知创建了组件
	notifyAddComponent(component);
	return component;
}

void txComponentOwner::destroyComponent(txComponent* component)
{
	// 后序遍历销毁组件,从最底层组件开始销毁,此处不能用引用获得子组件列表,因为在销毁组件过程中会对列表进行修改
	std::vector<txComponent*> children = component->getChildComponentList();
	int childCount = children.size();
	for (int i = 0; i < childCount; ++i)
	{
		destroyComponent(children[i]);
	}
	mComponentFactoryManager->getFactory(component->getType())->destroyComponent(component);
}

void txComponentOwner::destroyComponent(const std::string& name)
{
	// 在总列表中查找
	std::map<std::string, txComponent*>::iterator itrFind = mAllComponentList.find(name);
	if (itrFind == mAllComponentList.end())
	{
		return;
	}
	destroyComponent(itrFind->second);
}

void txComponentOwner::destroyAllComponents()
{
	std::map<std::string, std::map<std::string, txComponent*> >::iterator iterType = mAllComponentTypeList.begin();
	std::map<std::string, std::map<std::string, txComponent*> >::iterator iterTypeEnd = mAllComponentTypeList.end();
	for (; iterType != iterTypeEnd; ++iterType)
	{
		txComponentFactoryBase* factory = mComponentFactoryManager->getFactory(iterType->first);
		if (factory == NULL)
		{
			continue;
		}
		// 因为在销毁过程中会修改列表,复制一份是为了避免迭代器失效
		std::map<std::string, txComponent*> componentList = iterType->second;
		std::map<std::string, txComponent*>::iterator iterCom = componentList.begin();
		std::map<std::string, txComponent*>::iterator iterComEnd = componentList.end();
		for (; iterCom != iterComEnd; ++iterCom)
		{
			factory->destroyComponent(iterCom->second);
		}
	}
}

txComponent* txComponentOwner::getFirstActiveComponentByBaseType(const std::string& type)
{
	std::map<std::string, std::map<std::string, txComponent*> >::iterator iterBaseType = mAllComponentBaseTypeList.find(type);
	if (iterBaseType != mAllComponentBaseTypeList.end())
	{
		std::map<std::string, txComponent*>::iterator iterTypeCom = iterBaseType->second.begin();
		std::map<std::string, txComponent*>::iterator iterTypeComEnd = iterBaseType->second.end();
		for (; iterTypeCom != iterTypeComEnd; ++iterTypeCom)
		{
			txComponent* component = iterTypeCom->second;
			if (component->isActive() && !component->isLockOneFrame())
			{
				return component;
			}
		}
	}
	return NULL;
}

txComponent* txComponentOwner::getFirstActiveComponent(const std::string& type)
{
	std::map<std::string, std::map<std::string, txComponent*> >::iterator iter = mAllComponentTypeList.find(type);
	if (iter != mAllComponentTypeList.end())
	{
		std::map<std::string, txComponent*>::iterator iterTypeCom = iter->second.begin();
		std::map<std::string, txComponent*>::iterator iterTypeComEnd = iter->second.end();
		for (; iterTypeCom != iterTypeComEnd; ++iterTypeCom)
		{
			txComponent* component = iterTypeCom->second;
			if (component->isActive() && !component->isLockOneFrame())
			{
				return component;
			}
		}
	}
	return NULL;
}

void txComponentOwner::addComponentToList(txComponent* component, const int& componentPos)
{
	const std::string& name = component->getName();
	const std::string& type = component->getType();
	const std::string& baseType = component->getBaseType();

	// 如果没有父组件,则将组件放入第一级组件列表中
	if (component->getParentComponent() == NULL)
	{
		if (componentPos == -1)
		{
			mRootComponentList.push_back(component);
		}
		else
		{
			mRootComponentList.insert(mRootComponentList.begin() + componentPos, component);
		}
	}

	// 添加到组件列表中
	mAllComponentList.insert(std::make_pair(name, component));

	// 添加到组件类型分组列表中
	std::map<std::string, std::map<std::string, txComponent*> >::iterator iterType = mAllComponentTypeList.find(type);
	if (iterType != mAllComponentTypeList.end())
	{
		iterType->second.insert(std::make_pair(name, component));
	}
	else
	{
		std::map<std::string, txComponent*> componentList;
		componentList.insert(std::make_pair(name, component));
		mAllComponentTypeList.insert(std::make_pair(type, componentList));
	}

	// 添加到基础组件类型分组列表中
	std::map<std::string, std::map<std::string, txComponent*> >::iterator iterBaseType = mAllComponentBaseTypeList.find(baseType);
	if (iterBaseType != mAllComponentBaseTypeList.end())
	{
		iterBaseType->second.insert(std::make_pair(name, component));
	}
	else
	{
		std::map<std::string, txComponent*> componentList;
		componentList.insert(std::make_pair(name, component));
		mAllComponentBaseTypeList.insert(std::make_pair(baseType, componentList));
	}
}

void txComponentOwner::removeComponentFromList(txComponent* component)
{
	// 从第一级组件列表中移除
	if (component->getParentComponent() == NULL)
	{
		int componentCount = mRootComponentList.size();
		for (int i = 0; i < componentCount; ++i)
		{
			if (mRootComponentList[i] == component)
			{
				mRootComponentList.erase(mRootComponentList.begin() + i);
				break;
			}
		}
	}

	// 从所有组件列表中移除
	const std::string& componentName = component->getName();
	std::map<std::string, txComponent*>::iterator iterCom = mAllComponentList.find(componentName);
	if (iterCom != mAllComponentList.end())
	{
		mAllComponentList.erase(iterCom);
	}

	// 从组件类型分组列表中移除
	const std::string& realType = component->getType();
	std::map<std::string, std::map<std::string, txComponent*> >::iterator iterType = mAllComponentTypeList.find(realType);
	if (iterType != mAllComponentTypeList.end())
	{
		std::map<std::string, txComponent*>::iterator iterCom = iterType->second.find(componentName);
		if (iterCom != iterType->second.end())
		{
			iterType->second.erase(iterCom);
		}
	}

	// 从基础组件类型分组列表中移除
	const std::string& baseType = component->getBaseType();
	std::map<std::string, std::map<std::string, txComponent*> >::iterator iterBaseType = mAllComponentBaseTypeList.find(baseType);
	if (iterBaseType != mAllComponentBaseTypeList.end())
	{
		std::map<std::string, txComponent*>::iterator iter = iterBaseType->second.find(componentName);
		if (iter != iterBaseType->second.end())
		{
			iterBaseType->second.erase(iter);
		}
	}
}