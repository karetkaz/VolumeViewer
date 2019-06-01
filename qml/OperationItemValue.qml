import QtQuick 2.9
import QtQuick.Controls 1.4

OperationItem {

	property real value: 0;

	property alias label: idSize.text
	property alias stepSize: idSize.stepSize
	property alias precision: idSize.precision
	property alias minimumValue: idSize.minimumValue
	property alias maximumValue: idSize.maximumValue

	SliderRow {
		id: idSize
		text: 'Value';
		textWidth: parent.labelWidth
		visible: parent.enabled
		spacing: parent.spacing

		value: parent.value
		onValueUpdated: parent.value = value
		updateOnRelease: parent.updateOnRelease
	}
}
