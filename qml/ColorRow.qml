import QtQuick 2.9
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2

Item {
	id: root
	property int spacing: 15

	property alias text: label.text
	property alias textWidth: label.width

	property alias value: colorDialog.color
	property alias hasAlpha: colorDialog.showAlphaChannel

	signal valueUpdated(color value);

	width: parent.width - (parent.leftPadding + parent.rightPadding)
	height: label.height * 1.3

	ColorDialog {
		id: colorDialog
		title: "Choose a color"
		onAccepted: root.valueUpdated(color)
	}

	Label {
		id: label
		horizontalAlignment: Text.AlignRight
		anchors {
			left: parent.left
			verticalCenter: root.verticalCenter
		}
	}

	Button {
		id: reset
		width: label.height * 3
		height: label.height
		text: '...'
		clip: true
		activeFocusOnTab: false
		anchors {
			left: label.right
			leftMargin: root.spacing
			verticalCenter: root.verticalCenter
		}
		onClicked: colorDialog.open()
	}
	Rectangle {
		color: colorDialog.color
		height: label.height
		border.color: "black"
		anchors {
			left: reset.visible ? reset.right : label.right
			right: parent.right
			leftMargin: root.spacing
			verticalCenter: root.verticalCenter
		}
		Label {
			id: colValue
			anchors.fill: parent
			text: '' + colorDialog.color
			verticalAlignment: Text.AlignVCenter
			horizontalAlignment: Text.AlignHCenter
		}
	}
}
