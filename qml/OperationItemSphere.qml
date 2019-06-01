import QtQuick 2.9
import QtQuick.Controls 1.4

OperationItem {

	readonly property real minValue: -2;
	readonly property real maxValue: +3;
	property alias minRadius: radius.minimumValue;
	property alias maxRadius: radius.maximumValue;

	property real _x: 0;
	property real _y: 0;
	property real _z: 0;
	property real _r: 0;

	SliderRow {
		text: 'Position.x';
		textWidth: parent.labelWidth
		visible: parent.enabled
		spacing: parent.spacing

		value: parent._x
		minimumValue: parent.minValue
		maximumValue: parent.maxValue
		onValueUpdated: parent._x = value
		updateOnRelease: parent.updateOnRelease
	}
	SliderRow {
		text: 'Position.y'
		textWidth: parent.labelWidth
		visible: parent.enabled
		spacing: parent.spacing

		value: parent._y
		minimumValue: parent.minValue
		maximumValue: parent.maxValue
		onValueUpdated: parent._y = value
		updateOnRelease: parent.updateOnRelease
	}
	SliderRow {
		text: 'Position.z'
		textWidth: parent.labelWidth
		visible: parent.enabled
		spacing: parent.spacing

		value: parent._z
		minimumValue: parent.minValue
		maximumValue: parent.maxValue
		onValueUpdated: parent._z = value
		updateOnRelease: parent.updateOnRelease
	}
	SliderRow {
		id: radius
		text: 'Radius'
		textWidth: parent.labelWidth
		visible: parent.enabled
		spacing: parent.spacing

		value: parent._r
		minimumValue: 1 / 1024
		maximumValue: parent.maxValue
		onValueUpdated: parent._r = value
		updateOnRelease: parent.updateOnRelease
	}
}
