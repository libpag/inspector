import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.impl

Rectangle {
    property alias text: itemText.text

    height: parent.height
    Text {
        id: itemText
        leftPadding: 10
        height: parent.height
        width: parent.width
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        color: "#DDDDDD"
        font.pixelSize: 18
        elide: Text.ElideRight
    }
}