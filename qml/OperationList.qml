import QtQuick 2.9
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2

ListView {
	property int labelWidth: 0;

	id: root
	clip: true
	width: parent.width

	property bool updateOnRelease: false;
	property alias count: operationListModel.count;
	signal valueChanged(variant item, int index, string field);

	function clear() {
		operationListModel.js = [];
		operationListModel.clear();
		root.currentIndex = -1;
	}

	function append(item) {
		operationListModel.js.push(item);
		return operationListModel.append(item);
	}

	function remove(index) {
		operationListModel.js.splice(index, 1);
		operationListModel.remove(index);
		root.currentIndex = -1;
	}

	function update(index, field, value, fromUser) {
		if (index < 0 || index >= operationListModel.js.length) {
			return;
		}

		root.currentIndex = index;
		if (field !== null) {
			root.get(index)[field] = value;
		}
		if (fromUser !== false) {
			root.valueChanged(root.get(index), index, field);
		}
	}
	
	function enable(index, value, fromUser) {
		if (index < 0 || index >= operationListModel.js.length) {
			return;
		}
		root.get(index).enabled = value;
	}

	function get(index, field, invalid) {
		if (index < 0 || index >= operationListModel.js.length) {
			if (invalid === undefined) {
				return {};
			}
			return invalid;
		}
		if (field === undefined) {
			return operationListModel.js[index];
		}
		if (operationListModel.js[index] === undefined) {
			return invalid;
		}
		return operationListModel.js[index][field];
	}

	model: ListModel {
		property var js: [];
		id: operationListModel
	}

	highlight: Component {
		Rectangle {
			color: settings.getValue('highlight', 'color', '#0dd');
			y: root.currentItem.y
			width: root.currentItem.width
			height: root.currentItem.height
			radius: 5
		}
	}
	keyNavigationWraps: false
	keyNavigationEnabled: false
	snapMode: ListView.SnapToItem
	highlightFollowsCurrentItem: false

	delegate: Loader {

		Component {
			id: defaultComponent
			OperationItem {
				text: name
				width: root.width
				labelWidth: root.labelWidth
				spacing: root.spacing
				enabled: root.get(index).enabled || false
				updateOnRelease: root.updateOnRelease

				onRemove: root.remove(index)
				onPreview: root.update(index, field, null)
				onEnabledChanged: root.enable(index, enabled, completed)
			}
		}

		Component {
			id: filterComponent
			OperationItemValue {
				text: name
				width: root.width
				labelWidth: root.labelWidth
				spacing: root.spacing
				enabled: root.get(index).enabled || false
				updateOnRelease: root.updateOnRelease

				label: 'Size'
				precision: 0
				stepSize: 2
				minimumValue: 3
				maximumValue: 13
				value: root.get(index).size;

				onRemove: root.remove(index)
				onPreview: root.update(index, field, null)
				onEnabledChanged: root.enable(index, enabled, completed)
				onValueChanged: root.update(index, 'size', value, completed)
			}
		}

		Component {
			id: customFilterComponent

			OperationItem {
				id: customFilterRoot
				text: name
				width: root.width
				labelWidth: root.labelWidth
				spacing: root.spacing
				enabled: root.get(index).enabled || false
				updateOnRelease: root.updateOnRelease

				property var avalues: [];
				readonly property int modelIndex: index

				onRemove: root.remove(index)
				onPreview: root.update(index, field, null)
				onEnabledChanged: root.enable(index, enabled, completed)

				SliderRow {
					id: customFilterSize

					text: 'Size';
					textWidth: labelWidth
					visible: parent.enabled
					spacing: parent.spacing

					value: root.get(index).size || 0;
					stepSize: 2
					minimumValue: 3
					maximumValue: 7

					property int sqareValue: value * value;
					property int cubeValue: value * sqareValue;

					onValueUpdated: root.update(index, 'size', this.value = value);
					onValueChanged: {
						sqareValue = value * value;
						cubeValue = value * sqareValue;
					}
				}

				// TODO: find a better way to do it
				Grid {
					rows: customFilterSize.value
					rowSpacing: 5
					Repeater {
						model: customFilterSize.value
						Grid {
							readonly property int aindex: index
							rows: customFilterSize.value
							columns: customFilterSize.value
							Repeater {
								model: customFilterSize.value * customFilterSize.value
								SpinBox {
									readonly property int aindex: customFilterSize.sqareValue * parent.aindex + index
									value: root.get(modelIndex).values[aindex] || 0;
									minimumValue: -99
									maximumValue: +99
									decimals: 2;
									onValueChanged: {
										customFilterRoot.avalues[aindex] = value;

										var values = [];
										for (var i = 0; i < customFilterRoot.avalues.length; i += 1) {
											values[i] = +customFilterRoot.avalues[i] || 0;
										}
										root.update(modelIndex, 'values', values);
									}
									Component.onCompleted: {
										customFilterRoot.avalues.length = customFilterSize.cubeValue;
										customFilterRoot.avalues[aindex] = value;
									}
								}
							}
						}
					}
					visible: parent.enabled
				}
			}
		}

		Component {
			id: thresholdComponent
			OperationItem {
				text: name
				width: root.width
				labelWidth: root.labelWidth
				spacing: root.spacing
				enabled: root.get(index).enabled || false
				updateOnRelease: root.updateOnRelease

				property real min: root.get(index).min
				property real max: root.get(index).max
				property bool norm: root.get(index).norm
				
				SliderRow {
					text: 'Minimum'
					textWidth: parent.labelWidth
					visible: parent.enabled
					spacing: parent.spacing

					value: parent.min
					minimumValue: 0
					maximumValue: 1
					onValueUpdated: parent.min = value
					updateOnRelease: parent.updateOnRelease
				}

				SliderRow {
					text: 'Maximum'
					textWidth: parent.labelWidth
					visible: parent.enabled
					spacing: parent.spacing

					value: parent.max
					minimumValue: 0
					maximumValue: 1
					onValueUpdated: parent.max = value
					updateOnRelease: parent.updateOnRelease
				}

				SliderRow {
					text: 'Normalize'
					textWidth: parent.labelWidth
					visible: parent.enabled
					spacing: parent.spacing

					value: parent.norm ? 1 : 0
					minimumValue: 0
					maximumValue: 1
					precision: 0
					stepSize: 1
					onValueUpdated: parent.norm = value != 0
					updateOnRelease: parent.updateOnRelease
				}

				onRemove: root.remove(index)
				onPreview: root.update(index, field, null)
				onEnabledChanged: root.enable(index, enabled, completed)
				onMinChanged: root.update(index, 'min', min, completed)
				onMaxChanged: root.update(index, 'max', max, completed)
				onNormChanged: root.update(index, 'norm', norm, completed)
			}
		}

		Component {
			id: cutCropSphereComponent
			OperationItemSphere {
				text: name
				width: root.width
				labelWidth: root.labelWidth
				spacing: root.spacing
				enabled: root.get(index).enabled || false
				updateOnRelease: root.updateOnRelease

				_x: root.get(index).x
				_y: root.get(index).y
				_z: root.get(index).z
				_r: root.get(index).r

				onRemove: root.remove(index)
				onPreview: root.update(index, field, null)
				onEnabledChanged: root.enable(index, enabled, completed)
				on_XChanged: root.update(index, 'x', _x, completed)
				on_YChanged: root.update(index, 'y', _y, completed)
				on_ZChanged: root.update(index, 'z', _z, completed)
				on_RChanged: root.update(index, 'r', _r, completed)
			}
		}

		Component {
			id: claheComponent
			OperationItem {
				text: name
				width: root.width
				labelWidth: root.labelWidth
				spacing: root.spacing
				enabled: root.get(index).enabled || false
				updateOnRelease: root.updateOnRelease

				property real bins: root.get(index).bins
				property real windowSize: root.get(index).windowSize
				property real clipLimit: root.get(index).clipLimit

				onRemove: root.remove(index)
				onPreview: root.update(index, field, null)
				onEnabledChanged: root.enable(index, enabled, completed)
				onBinsChanged: root.update(index, 'bins', bins, completed)
				onWindowSizeChanged: root.update(index, 'windowSize', windowSize, completed)
				onClipLimitChanged: root.update(index, 'clipLimit', clipLimit, completed)

				SliderRow {
					text: 'Bins'
					textWidth: parent.labelWidth
					visible: parent.enabled
					spacing: parent.spacing

					value: parent.bins
					minimumValue: 1
					maximumValue: 256
					precision: 0
					onValueUpdated: parent.bins = value
					updateOnRelease: parent.updateOnRelease
				}
				SliderRow {
					text: 'Window Size'
					textWidth: parent.labelWidth
					visible: parent.enabled
					spacing: parent.spacing

					value: parent.windowSize
					minimumValue: 0
					maximumValue: 256
					precision: 0
					onValueUpdated: parent.windowSize = value
					updateOnRelease: parent.updateOnRelease
				}
				SliderRow {
					text: 'Clip Limit'
					textWidth: parent.labelWidth
					visible: parent.enabled
					spacing: parent.spacing
					
					value: parent.clipLimit
					minimumValue: 0
					maximumValue: 10
					onValueUpdated: parent.clipLimit = value
					updateOnRelease: parent.updateOnRelease
				}
			}
		}

		property var delegateMap: {
			'CropSphere': cutCropSphereComponent,
			'CutSphere': cutCropSphereComponent,
			'Threshold': thresholdComponent,
			'BoxBlur': filterComponent,
			'GaussBlur': filterComponent,
			'CustomFilter': customFilterComponent,
			'Erode': filterComponent,
			'Dilate': filterComponent,
			'Median': filterComponent,
			'Clahe': claheComponent,
		}
		sourceComponent: delegateMap[name] || defaultComponent
	}
}
