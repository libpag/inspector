import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.impl
import MeshDrawer 1.0

Item {
    id: root
    width: 1120
    height: 800

    Rectangle {
        anchors.fill: parent
        color: "#383838"
        z: -1
    }

    SplitView {
        id: splitView
        anchors.fill: parent
        width: parent.width
        height: parent.height
        orientation: Qt.Vertical

        handle: Rectangle {
            implicitHeight: 2
            color: "#383838"
        }

        ScrollView {
            id: scrollView
            SplitView.fillWidth: true
            implicitHeight: splitView.height / 2

            Row {
                id: meshLayout
                width: parent.width
                height: scrollView.height
                spacing: 2
                clip: true

                property int minItemWidth: 180
                property var syncContentY: 0
                onSyncContentYChanged: {
                    for (var i = 0; i < repeater.count; ++i) {
                        repeater.itemAt(i).meshListSyncContentY = syncContentY
                    }
                }

                property var selectIndex: -1
                onSelectIndexChanged: {
                    meshModel.setSelectMeshItem(selectIndex)
                    for (var i = 0; i < repeater.count; ++i) {
                        repeater.itemAt(i).selectIndex = selectIndex
                    }
                }

                Repeater {
                    id: repeater
                    model: {
                        if (meshModel === null) {
                            return 0
                        }
                        return meshModel.itemSize
                    }
                    MeshListItem {
                        property var valueCount: meshModel.itemValueCount[index]

                        width: valueCount * meshLayout.minItemWidth
                        height: parent.height

                        meshListSyncContentY: meshLayout.syncContentY
                        onMeshListSyncContentYChanged: {
                            if (meshListSyncContentY !== meshLayout.syncContentY) {
                                meshLayout.syncContentY = meshListSyncContentY
                            }
                        }

                        selectIndex: meshLayout.selectIndex
                        onSelectIndexChanged: {
                            if (selectIndex !== meshLayout.selectIndex) {
                                meshLayout.selectIndex = selectIndex
                            }
                        }
                        meshLabelName: {
                            if (meshModel === null) {
                                return ""
                            }
                            return meshModel.names[index]
                        }
                        meshDataModel: {
                            if (meshModel === null) {
                                return []
                            }
                            return meshModel.values[index]
                        }
                    }
                }

                Rectangle {
                    width: {
                        var size = meshModel === null ? 0 : meshModel.itemSize
                        var repeaterWidth = 0
                        for (var i = 0; i < size; ++i) {
                            repeaterWidth += meshModel.itemValueCount[i] * meshLayout.minItemWidth
                        }
                        var width = root.width - repeaterWidth
                        return width > 0 ? width : 0
                    }
                    height: parent.height
                    color: "#535353"
                }
            }
        }

        Rectangle {
            id: wireFrameArea
            SplitView.fillWidth: true
            implicitHeight: splitView.height / 2
            color: "#282828"

            MeshDrawer {
                id: meshDrawer
                anchors.fill: parent
                width: wireFrameArea.width
                height: wireFrameArea.height
                worker: workerPtr
                viewData: viewDataPtr
                objectName: "meshDrawer"
            }
            // WireFrameDrawer {
            //     id: wf
            //     x: 100; y: 50
            //     width: 400; height: 300
            //     lineColor: "cyan"
            //     lineWidth: 2.0
            // }
            //
            // Component.onCompleted: {
            //     var vertices = [
            //         0, 0,
            //         200, 0,
            //         200, 150,
            //         0, 150
            //     ];
            //     var indices = [0,1,2, 2,3,0];
            //     wf.setGeometry(vertices, indices);
            // }
        }
    }
}