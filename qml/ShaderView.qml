import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.impl

Item {
    id: root
    width: 1120
    height: 800

    Rectangle {
        anchors.fill: parent
        color: "#383838"
        z: -1
    }

    ColumnLayout {
        anchors.fill: parent
        width: parent.width
        height: parent.height
        spacing: 2

        RowLayout {
            id: shaderTextLayout
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 2

            ShaderText {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: parent.width / 2
                rightMargin: 2
                shaderCode: {
                    if (shaderTextModel === null) {
                        return ""
                    }
                    return shaderTextModel.vertexShaderText
                }
                shaderLabel: "Vertex Shader"
            }

            ShaderText {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: parent.width / 2
                leftMargin: 2
                shaderCode: {
                    if (shaderTextModel === null) {
                        return ""
                    }
                    return shaderTextModel.fragmentShaderText
                }
                shaderLabel: "Fragment Shader"
            }
        }

        Column {
            id: uniformDataLayout
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(200, 27 + uniformDataView.contentHeight)
            spacing: 2

            Rectangle {
                id: uniformHeadline
                color: "#535353"
                width: parent.width
                height: 25

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: 14
                    color: "#DDDDDD"
                    text: "Uniform"
                }
            }

            ListView {
                id: uniformDataView
                width: parent.width
                height: Math.min(200 - 27, contentHeight)
                clip: true
                model: {
                    if (uniformModel === null) {
                        return []
                    }
                    return uniformModel.uniformItems
                }
                spacing: 1

                delegate: Rectangle {
                    width: ListView.view.width
                    height: 32 + (modelData.height - 1) * 15
                    color: "#383838"

                    Row {
                        anchors.fill: parent
                        spacing: 1

                        ListViewItem {
                            width: parent.width * 0.2
                            text: modelData.name
                            color: "#535353"
                        }

                        ListViewItem {
                            width: parent.width * 0.6
                            text: modelData.value
                            color: "#535353"
                        }

                        ListViewItem {
                            width: parent.width * 0.2
                            text: modelData.format
                            color: "#535353"
                        }
                    }
                }
            }
        }
    }
}