/****************************************************************************
**
** Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt3D module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "skeleton_p.h"

#include <Qt3DCore/qjoint.h>
#include <Qt3DCore/qpropertyupdatedchange.h>

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>

#include <Qt3DRender/private/abstractrenderer_p.h>
#include <Qt3DRender/private/gltfskeletonloader_p.h>
#include <Qt3DRender/private/managers_p.h>
#include <Qt3DRender/private/nodemanagers_p.h>
#include <Qt3DRender/private/renderlogging_p.h>
#include <Qt3DRender/private/qurlhelper_p.h>
#include <Qt3DCore/private/qskeletoncreatedchange_p.h>
#include <Qt3DCore/private/qskeletonloader_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt3DCore;

namespace Qt3DRender {
namespace Render {

Skeleton::Skeleton()
    : BackendNode(Qt3DCore::QBackendNode::ReadWrite)
    , m_status(Qt3DCore::QSkeletonLoader::NotReady)
{
}

void Skeleton::cleanup()
{
    m_source.clear();
    m_status = Qt3DCore::QSkeletonLoader::NotReady;
    clearData();
    setEnabled(false);
}

void Skeleton::initializeFromPeer(const Qt3DCore::QNodeCreatedChangeBasePtr &change)
{
    m_skeletonManager = m_renderer->nodeManagers()->skeletonManager();
    const auto skeletonCreatedChange = qSharedPointerCast<QSkeletonCreatedChangeBase>(change);
    switch (skeletonCreatedChange->type()) {
    case QSkeletonCreatedChangeBase::SkeletonLoader: {
        const auto loaderTypedChange = qSharedPointerCast<QSkeletonCreatedChange<QSkeletonLoaderData>>(change);
        const auto &data = loaderTypedChange->data;
        m_dataType = File;
        m_source = data.source;
        m_createJoints = data.createJoints;
        if (!m_source.isEmpty()) {
            markDirty(AbstractRenderer::SkeletonDataDirty);
            m_skeletonManager->addDirtySkeleton(SkeletonManager::SkeletonDataDirty, peerId());
        }
        break;
    }

    case QSkeletonCreatedChangeBase::Skeleton:
        // TODO: Handle QSkeleton case (non loaded)
        break;
    }
}

void Skeleton::sceneChangeEvent(const Qt3DCore::QSceneChangePtr &e)
{
    switch (e->type()) {
    case Qt3DCore::PropertyUpdated: {
        const auto change = qSharedPointerCast<QPropertyUpdatedChange>(e);
        if (change->propertyName() == QByteArrayLiteral("source")) {
            Q_ASSERT(m_dataType == File);
            const auto source = change->value().toUrl();
            if (source != m_source) {
                m_source = source;
                markDirty(AbstractRenderer::SkeletonDataDirty);
                m_skeletonManager->addDirtySkeleton(SkeletonManager::SkeletonDataDirty, peerId());
            }
        } else if (change->propertyName() == QByteArrayLiteral("createJointsEnabled")) {
            m_createJoints = change->value().toBool();
        }

        // TODO: Handle QSkeleton case (non loaded)

        break;
    }

    default:
        break;
    }
    QBackendNode::sceneChangeEvent(e);
}

void Skeleton::setStatus(QSkeletonLoader::Status status)
{
    if (status != m_status) {
        m_status = status;
        Qt3DCore::QPropertyUpdatedChangePtr e = Qt3DCore::QPropertyUpdatedChangePtr::create(peerId());
        e->setDeliveryFlags(Qt3DCore::QSceneChange::DeliverToAll);
        e->setPropertyName("status");
        e->setValue(QVariant::fromValue(m_status));
        notifyObservers(e);
    }
}

void Skeleton::notifyJointCount()
{
    Qt3DCore::QPropertyUpdatedChangePtr e = Qt3DCore::QPropertyUpdatedChangePtr::create(peerId());
    e->setDeliveryFlags(Qt3DCore::QSceneChange::DeliverToAll);
    e->setPropertyName("jointCount");
    e->setValue(jointCount());
    notifyObservers(e);
}

void Skeleton::loadSkeleton()
{
    qCDebug(Jobs) << Q_FUNC_INFO << m_source;
    clearData();

    // Load the data
    switch (m_dataType) {
    case File:
        loadSkeletonFromUrl();
        break;

    case Data:
        loadSkeletonFromData();
        break;

    default:
        Q_UNREACHABLE();
    }

    // If using a loader inform the frontend of the status change.
    // Don't bother if asked to create frontend joints though. When
    // the backend gets notified of those joints we'll update the
    // status at that point.
    if (m_dataType == File && !m_createJoints) {
        if (jointCount() == 0)
            setStatus(QSkeletonLoader::Error);
        else
            setStatus(QSkeletonLoader::Ready);
    }
    notifyJointCount();

    qCDebug(Jobs) << "Loaded skeleton data:" << *this;
}

void Skeleton::loadSkeletonFromUrl()
{
    // TODO: Handle remote files
    QString filePath = Qt3DRender::QUrlHelper::urlToLocalFileOrQrc(m_source);
    QFileInfo info(filePath);
    if (!info.exists()) {
        qWarning() << "Could not open skeleton file:" << filePath;
        setStatus(Qt3DCore::QSkeletonLoader::Error);
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open skeleton file:" << filePath;
        setStatus(Qt3DCore::QSkeletonLoader::Error);
        return;
    }

    // TODO: Make plugin based for more file type support. For now gltf or native
    const QString ext = info.suffix();
    if (ext == QLatin1String("gltf")) {
        GLTFSkeletonLoader loader;
        loader.load(&file);
        m_skeletonData = loader.createSkeleton(m_name);

        // If the user has requested it, create the frontend nodes for the joints
        // and send them to the (soon to be owning) QSkeletonLoader.
        if (m_createJoints) {
            std::unique_ptr<QJoint> rootJoint(createFrontendJoints(m_skeletonData));
            if (!rootJoint) {
                qWarning() << "Failed to create frontend joints";
                setStatus(Qt3DCore::QSkeletonLoader::Error);
                return;
            }

            // Move the QJoint tree to the main thread and notify the
            // corresponding QSkeletonLoader
            const auto appThread = QCoreApplication::instance()->thread();
            rootJoint->moveToThread(appThread);

            auto e = QJointChangePtr::create(peerId());
            e->setDeliveryFlags(Qt3DCore::QSceneChange::Nodes);
            e->setPropertyName("rootJoint");
            e->data = std::move(rootJoint);
            notifyObservers(e);

            // Clear the skeleton data. It will be recreated from the
            // frontend joints. A little bit inefficient but ensures
            // that joints created this way and via QSkeleton go through
            // the same code path.
            m_skeletonData = SkeletonData();
        }
    } else if (ext == QLatin1String("json")) {
        // TODO: Support native skeleton type
    } else {
        qWarning() << "Unknown skeleton file type:" << ext;
        setStatus(Qt3DCore::QSkeletonLoader::Error);
        return;
    }
    m_skinningPalette.resize(m_skeletonData.joints.size());
}

void Skeleton::loadSkeletonFromData()
{

}

Qt3DCore::QJoint *Skeleton::createFrontendJoints(const SkeletonData &skeletonData) const
{
    if (skeletonData.joints.isEmpty())
        return nullptr;

    // Create frontend joints from the joint info objects
    QVector<QJoint *> frontendJoints;
    frontendJoints.reserve(skeletonData.joints.size());
    for (const JointInfo &jointInfo : skeletonData.joints)
        frontendJoints.push_back(createFrontendJoint(jointInfo));

    // Now go through and resolve the parent for each joint
    for (int i = 0; i < frontendJoints.size(); ++i) {
        const auto parentIndex = skeletonData.joints[i].parentIndex;
        if (parentIndex == -1)
            continue;

        // It's not enough to just set up the QObject parent-child relationship.
        // We need to explicitly add the child to the parent's list of joints so
        // that information is then propagated to the backend.
        frontendJoints[parentIndex]->addChildJoint(frontendJoints[i]);
    }

    return frontendJoints[0];
}

Qt3DCore::QJoint *Skeleton::createFrontendJoint(const JointInfo &jointInfo) const
{
    auto joint = new QJoint();
    joint->setTranslation(jointInfo.localPose.translation);
    joint->setRotation(jointInfo.localPose.rotation);
    joint->setScale(jointInfo.localPose.scale);
    joint->setInverseBindMatrix(jointInfo.inverseBindPose);
    // TODO: Add name property to joint
    return joint;
}

void Skeleton::clearData()
{
    m_name.clear();
    m_skeletonData.joints.clear();
}

QVector<QMatrix4x4> Skeleton::calculateSkinningMatrixPalette()
{
    for (int i = 0; i < m_skeletonData.joints.size(); ++i) {
        // Calculate the global pose of this joint
        JointInfo &joint = m_skeletonData.joints[i];
        if (joint.parentIndex == -1) {
            joint.globalPose = joint.localPose.toMatrix();
        } else {
            JointInfo &parentJoint = m_skeletonData.joints[joint.parentIndex];
            joint.globalPose = parentJoint.globalPose * joint.localPose.toMatrix();
        }

        m_skinningPalette[i] = joint.globalPose * joint.inverseBindPose;
    }
    return m_skinningPalette;
}

} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE