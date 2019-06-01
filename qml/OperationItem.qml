import QtQuick 2.9
import QtQuick.Controls 1.4

Column {
	id: root
	padding: 3
	width: parent.width

	property int spacing: 0
	property int labelWidth: 0

	property alias text: label.text
	property alias enabled: enable.checked
	property alias pressed: enable.pressed

	property bool completed: false
	property bool updateOnRelease: false

	signal remove();
	signal preview(string field);

	Item {
		width: parent.width - (parent.leftPadding + parent.rightPadding)
		height: remove.height
		CheckBox {
			id: enable
			activeFocusOnTab: false
			activeFocusOnPress: false
			height: preview.height
			anchors.left: parent.left;
			anchors.verticalCenter: remove.verticalCenter
		}
		Label {
			id: label
			anchors.left: enable.right
			anchors.leftMargin: spacing
			anchors.right: preview.left
			anchors.verticalCenter: remove.verticalCenter
			elide: Text.ElideMiddle
			verticalAlignment: Text.AlignVCenter
			height: preview.height
		}
		Button {
			id: preview
			text: 'Preview'
			anchors.right: remove.left
			onClicked: root.preview(text)
			anchors.verticalCenter: remove.verticalCenter
		}
		Button {
			id: remove
			text: 'Remove'
			anchors.right: parent.right
			onClicked: root.remove()
		}
	}
	Component.onCompleted: completed = true
}

