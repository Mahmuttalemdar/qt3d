/****************************************************************************
**
** Copyright (C) 2015 Klaralvdalens Datakonsult AB (KDAB).
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

import Qt3D.Core 2.0
import Qt3D.Render 2.0

FrameGraph {
    id: quadViewportFrameGraph

    property alias topLeftCamera: cameraSelectorTopLeftViewport.camera;
    property alias topRightCamera: cameraSelectorTopRightViewport.camera;
    property alias bottomLeftCamera: cameraSelectorBottomLeftViewport.camera;
    property alias bottomRightCamera: cameraSelectorBottomRightViewport.camera;
    property alias window: surfaceSelector.window

    RenderSurfaceSelector {
        id: surfaceSelector

        Viewport {
            id: mainViewport
            rect: Qt.rect(0, 0, 1, 1)

            ClearBuffer {
                buffers: ClearBuffer.ColorDepthBuffer
            }

            Viewport {
                id: topLeftViewport
                rect: Qt.rect(0, 0, 0.5, 0.5)
                CameraSelector { id: cameraSelectorTopLeftViewport }
            }

            Viewport {
                id: topRightViewport
                rect: Qt.rect(0.5, 0, 0.5, 0.5)
                CameraSelector { id: cameraSelectorTopRightViewport }
            }

            Viewport {
                id: bottomLeftViewport
                rect: Qt.rect(0, 0.5, 0.5, 0.5)
                CameraSelector { id: cameraSelectorBottomLeftViewport }
            }

            Viewport {
                id: bottomRightViewport
                rect: Qt.rect(0.5, 0.5, 0.5, 0.5)
                CameraSelector { id: cameraSelectorBottomRightViewport }
            }
        }
    }
}
