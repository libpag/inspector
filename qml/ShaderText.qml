import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.impl

Rectangle {
    id: root
    color: "#FFFFFF"
    property real rightMargin: 0
    property real leftMargin: 0
    property alias shaderCode: shaderText.text
    property alias shaderLabel: shaderLabel.text

    ColumnLayout {
        id: rootLayout
        anchors.left: parent.left
        anchors.fill: parent
        spacing: 0

        Rectangle {
            height: 25
            Layout.fillWidth: true
            color: "#535353"

            Row {
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.left: parent.left
                anchors.leftMargin: 10
                spacing: 10

                Item {
                    height: 20
                    width: 20
                    Image {
                        id: codeImage
                        source: "qrc:icons/code.png"
                        width: 20
                        height: 20
                        anchors.centerIn: parent
                    }
                }

                Label {
                    id: shaderLabel
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: 18
                    color: "#DDDDDD"
                }
            }
        }

        Rectangle {
            id: textBackgrpund
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#535353"

            Rectangle {
                anchors.fill: parent
                anchors.leftMargin: root.leftMargin
                anchors.rightMargin: root.rightMargin
                color: "#282828"

                ScrollView {
                    clip: true
                    anchors.fill: parent
                    anchors.margins: 5
                    background: Rectangle {
                        color: "#282828"
                    }

                    Text {
                        id: shaderText
                        width: parent.width
                        height: parent.height
                        font.pixelSize: 18
                        color: "#DDDDDD"
                    }
                }
            }
        }
    }
}
