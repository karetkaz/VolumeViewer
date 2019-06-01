import QtQuick 2.9
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

Item {
	id: root
	property int spacing: 15
	property int precision: 3
	property bool updateOnRelease: false

	property alias text: label.text
	property alias textWidth: label.width

	property alias value: slider.value
	property alias pressed: slider.pressed
	property alias stepSize: slider.stepSize
	property alias minimumValue: slider.minimumValue
	property alias maximumValue: slider.maximumValue
	property alias wheelEnabled: slider.wheelEnabled

	property alias checked: check.checked
	property alias checkable: check.visible
	property alias resetable: reset.visible

	property real keysStep: 10 / Math.pow(10, precision)

	property real originalValue: value

	width: parent.width - (parent.leftPadding + parent.rightPadding)
	height: label.height * 1.3

	signal valueUpdated(real value);

	Label {
		id: label
		horizontalAlignment: Text.AlignRight
		anchors {
			left: parent.left
			verticalCenter: root.verticalCenter
		}
	}
	Button {
		property real value: 0./0
		id: reset
		width: 3 * label.height
		height: label.height
		visible: this.value === this.value;
		text: slider.value.toFixed(precision)
		clip: true
		activeFocusOnTab: false
		anchors {
			left: label.right
			leftMargin: root.spacing
			verticalCenter: root.verticalCenter
		}
		onClicked: {
			slider.forceActiveFocus()
			root.valueUpdated(this.value)
		}
	}
	CheckBox {
		id: check
		visible: false
		height: root.height
		activeFocusOnTab: false
		activeFocusOnPress: false
		
		anchors {
			left: reset.visible ? reset.right : label.right
			leftMargin: root.spacing
			verticalCenter: root.verticalCenter
		}
		onCheckedChanged: {
			slider.forceActiveFocus()
			if (reset.value !== reset.value) {
				// set initial value (root.status !== Component.Ready)
				return;
			}
			root.valueUpdated(slider.value);
		}
	}

	Slider {
		id: slider
		height: label.height
		stepSize: 0
		wheelEnabled: false   // scrolling on the slider removes the binding to value
		tickmarksEnabled: false
		activeFocusOnPress: true

		anchors {
			left: check.visible ? check.right : reset.visible ? reset.right : label.right
			right: parent.right
			leftMargin: root.spacing
			verticalCenter: root.verticalCenter
		}

		onValueChanged: {
			if (!root.updateOnRelease || !pressed) {
				root.valueUpdated(value);
			}
		}
		onPressedChanged: if (root.updateOnRelease) {
			if (!pressed) {
				root.valueUpdated(value);
			}
		}
		Keys.enabled: false
	}

	Keys.onPressed: {
		function stepBy(by) {
			var val = value + by;
			if (val < minimumValue) {
				val = minimumValue;
			}
			else if (val > maximumValue) {
				val = maximumValue;
			}
			root.valueUpdated(val);
		}

		//operationLog.log("smallStep: " + ((maximumValue - minimumValue) / Math.pow(1, -precision)))

		switch (event.key) {
			case Qt.Key_Space:
				if (event.modifiers & Qt.ControlModifier) {
					root.valueUpdated(reset.value);
				}
				break;

			case Qt.Key_Home:
				root.valueUpdated(minimumValue);
				break;

			case Qt.Key_End:
				root.valueUpdated(maximumValue);
				break;

			case Qt.Key_PageUp:
				stepBy(+root.keysStep * 10);
				break;
			
			case Qt.Key_PageDown:
				stepBy(-root.keysStep * 10);
				break;

			case Qt.Key_Up:
			case Qt.Key_Right:
				if (event.modifiers & Qt.ShiftModifier) {
					stepBy(+root.keysStep / 10);
				} else {
					stepBy(+root.keysStep);
				}
				break;

			case Qt.Key_Down:
			case Qt.Key_Left:
				if (event.modifiers & Qt.ShiftModifier) {
					stepBy(-root.keysStep / 10);
				} else {
					stepBy(-root.keysStep);
				}
				break;
		}
	}
	Component.onCompleted: {
		// remove binding
		reset.value = slider.value;
	}
}
