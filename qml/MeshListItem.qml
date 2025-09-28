import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.impl

Column {
    id: meshItemColum
    spacing: 1

    property var meshListSyncContentY: 0
    property var selectIndex: -1
    property alias meshLabelName: meshLabel.text
    property var meshDataModel: []
    Rectangle {
        color: "#535353"
        width: parent.width
        height: 40

        Text {
            id: meshLabel
            leftPadding: 10
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            font.pixelSize: 14
            color: "#DDDDDD"
            elide: Text.ElideRight
        }
    }
    
    ListView {
        id: meshDataView
        width: parent.width
        height: Math.min(parent.height - 27, contentHeight)
        spacing: 1
        clip: true
        model: meshItemColum.meshDataModel
        contentY: meshItemColum.meshListSyncContentY
        onContentYChanged: {
            if (contentY !== meshItemColum.meshListSyncContentY) {
                meshItemColum.meshListSyncContentY = contentY
            }
        }

        currentIndex: meshItemColum.selectIndex
        onCurrentIndexChanged: {
            if (currentIndex !== meshItemColum.selectIndex) {
                meshItemColum.selectIndex = currentIndex
            }
        }

        delegate: RowLayout {
            id: valueItemLayout
            width: meshDataView.width
            height: 40
            spacing: 1

            property var currentIndex: index
            property var valueItem: model.modelData

            Repeater {
                model: {
                    if (valueItemLayout.valueItem == null) {
                        return 0
                    }
                    return valueItemLayout.valueItem.length
                }
                ListViewItem {
                    id: listViewItem
                    text: valueItemLayout.valueItem[index]
                    color: meshDataView.currentIndex === valueItemLayout.currentIndex ? "#6B6B6B" : "#535353"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            meshDataView.currentIndex = valueItemLayout.currentIndex
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        width: parent.width
        height: parent.height
        color: "#535353"
    }
}