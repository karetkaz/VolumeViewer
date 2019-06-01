import QtQuick 2.9
import QtQuick.Window 2.3
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.3
import VolumeRenderer 1.0

ApplicationWindow {
	id: root
	x: settings.getValue('AppWindow', 'x', 600);
	y: settings.getValue('AppWindow', 'y', 30);
	width: settings.getValue('AppWindow', 'w', 512);
	height: settings.getValue('AppWindow', 'h', 600);
	visible: true

	property int labelWidth: 60
	property int spacing: 6

	property real selX: .5;
	property real selY: .5;
	property real selZ: .5;
	property real selR: settings.getValue('highlight', 'size', 0.01);

	function saveSettings(selection) {
		if (selection === undefined || selection === 'general') {
			settings.setValue(null, 'volume.resolution', volumeResolution);
			settings.setValue(null, 'thumbnail.resolution', 128);
			settings.setValue(null, 'compute.threads', volume3dData.maxThreads);
		}

		if (selection === undefined || selection === 'layout') {
			settings.setValue('AppWindow', 'x', root.x);
			settings.setValue('AppWindow', 'y', root.y);
			settings.setValue('AppWindow', 'w', root.width);
			settings.setValue('AppWindow', 'h', root.height);
			settings.setValue('GlWindow', 'x', volume3dView.x);
			settings.setValue('GlWindow', 'y', volume3dView.y);
			settings.setValue('GlWindow', 'w', volume3dView.width);
			settings.setValue('GlWindow', 'h', volume3dView.height);
		}

		if (selection === undefined || selection === 'view') {
			settings.setValue('view', 'alpha', volume3dView.alpha);
			settings.setValue('view', 'threshold', volume3dView.threshold);
			settings.setValue('view', 'brightness', volume3dView.brightness);
			settings.setValue('view', 'contrast', volume3dView.contrast);
			settings.setValue('view', 'gamma', volume3dView.gamma);

			settings.setValue('view', 'zoom', volume3dView.zoom);
			settings.setValue('view', 'plane', volume3dView.plane);

			settings.setValue('highlight', 'border', volume3dView.highlightBorder);
			settings.setValue('highlight', 'color', volume3dView.highlightColor.toString());
			settings.setValue('highlight', 'size', root.selR);

			settings.setValue('GlWindow', 'background', volume3dView.backgroundColor.toString());
		}
	}

	ListModel {
		id: operationLog
		function toSeconds(time, pad) {
			var result = (time / 1000).toFixed(2);
			while (result.length < pad + 3) {
				result = '0' + result;
			}
			return result;
		}
		function asString(item) {
			return '[@' + toSeconds(item.time, 4) + ']: ' + item.text + (item.elapsed < 0 ? '' : ' completed in: ' + toSeconds(item.elapsed) + ' seconds');
		}

		function push(item) {
			this.insert(0, item);
			volume3dData.print(asString(item));
		}
		function d(message) {
			this.push({ time: volume3dData.time(), text: message, elapsed: -1, color: 'gray' });
		}
		function i(message) {
			this.push({ time: volume3dData.time(), text: message, elapsed: -1, color: 'blue' });
		}
		function e(message) {
			this.push({ time: volume3dData.time(), text: message, elapsed: -1, color: 'red' });
		}
	}

	Volume3dData {
		id: volume3dData
		maxThreads: 1

		onVolumeChanged: {
			operationLog.d('onVolumeChanged');
			volume3dView.visible = true;
			volume3dView.preview();
		}

		onOperationStart: {
			console.log('onOperationStart: ' + message);
			operationLog.push({ time: time, text: message, elapsed: -1 });
		}
		onOperationFailed: {
			operationLog.e(message);
		}
		onOperationComplete: {
			console.log('onOperationComplete: ' + message + '(time: ' + (elapsed / 1000).toFixed(2) + ' sec)');
			operationLog.push({ time: time, text: message, elapsed: elapsed, color: "green" });
		}
	}

	Volume3dWindow {
		id: volume3dView
		model: volume3dData

		x: settings.getValue('GlWindow', 'x', 0);
		y: settings.getValue('GlWindow', 'y', 30);
		width: settings.getValue('GlWindow', 'w', 600);
		height: settings.getValue('GlWindow', 'h', 600);
		backgroundColor: settings.getValue('GlWindow', 'background', backgroundColor);
		highlightColor: settings.getValue('highlight', 'color', highlightColor);
		highlightBorder: settings.getValue('highlight', 'border', highlightBorder);

		threshold: settings.getValue('view', 'threshold', this.getThreshold());
		alpha: settings.getValue('view', 'alpha', this.getAlpha());
		plane: settings.getValue('view', 'plane', this.getPlane());
		zoom: settings.getValue('view', 'zoom', this.getZoom());

		property int display: Volume3dData.Thumb;

		property real rotx: 0;
		property real roty: 0;
		property real rotz: 0;

		property real brightness: settings.getValue('view', 'brightness', 0);
		property real contrast: settings.getValue('view', 'contrast', 0);
		property real gamma: settings.getValue('view', 'gamma', 1);

		onMouseDrag: {
			function absmod(value, modulus) {
				if (value > modulus) {
					return value % modulus;
				}
				if (value < 0) {
					return modulus - value % modulus;
				}
				return value;
			}
			function clamp(value, min, max) {
				if (value < min) {
					value = min
				}
				else if (value > max) {
					value = max
				}
				return value;
			}

			var scale = 0.3 / zoom;
			if (btn == 1) {
				rotate(dx * scale, -dy * scale);
			}
			else if (btn == 2) {
				this.plane = clamp(this.plane - dy / height, -1, 0);
			}
			else if (btn == 4) {
				this.rotx = absmod(this.rotx + dx * scale, 360);
				this.roty = absmod(this.roty - dy * scale, 360);
			}
		}
		onMouseScroll: {
			if (dx == 0) {
				return;
			}

			var scale = dx * Math.SQRT2;
			if (scale > 0) {
				scale = zoom * scale;
				if (scale > 64) {
					scale = 64;
				}
			} else {
				scale = zoom / -scale;
				if (scale < 1) {
					scale = 1;
				}
			}
			this.zoom = scale;
		}
		onMouseSelect: {
			var f = 3;
			operationLog.d("poscol(r: " + r.toFixed(f) + ", g: " + g.toFixed(f) + ", b: " + b.toFixed(f) + ")");
			if (btn === Qt.LeftButton) {
				selX = r;
				selY = g;
				selZ = b;
				volume3dView.render(display, selX, selY, selZ, selR);
			}
		}

		onDisplayChanged: preview();
		onThresholdChanged: preview();
		onAlphaChanged: preview();

		onBrightnessChanged: adjust(brightness, contrast, gamma);
		onContrastChanged: adjust(brightness, contrast, gamma);
		onGammaChanged: adjust(brightness, contrast, gamma);

		onRotxChanged: {
			setTransform([
				1,0,0,0,
				0,1,0,0,
				0,0,1,0,
				0,0,0,1,
			]);
			rotate(rotx, roty, rotz)
		}
		onRotyChanged: {
			setTransform([
				1,0,0,0,
				0,1,0,0,
				0,0,1,0,
				0,0,0,1,
			]);
			rotate(rotx, roty, rotz)
		}
		onRotzChanged: {
			setTransform([
				1,0,0,0,
				0,1,0,0,
				0,0,1,0,
				0,0,0,1,
			]);
			rotate(rotx, roty, rotz)
		}

		onVisibleChanged: if (!visible) {
			Qt.quit();
		}

		function preview(mode, selection) {
			if (mode === undefined) {
				mode = volume3dView.display;
			}

			if (selR > 0 && selection !== false) {
				render(mode, selX, selY, selZ, selR);
			} else {
				render(mode);
			}
		}
	}

	Item {
		id: volume
		width: parent.width
		height: openVolumePath.height
		FileDialog {
			id: openVolumeDialog
			nameFilters: [
				'All files (*)',
				'Volume files (*.vol *.nii *.nii.gz)',
				'Image files (*.png *.tiff *.bmp *.jpg)'
			]
			onSelectionAccepted: {
				try {
					openVolumePath.text = fileUrl;
					volume3dData.open(fileUrl);
				} catch (err) {
					console.log(err);
				}
			}
		}

		FileDialog {
			id: saveVolumeDialog
			selectExisting: false
			selectMultiple: false

			nameFilters: [
				'Volume files (*.vol *.png)',
				'All files (*)'
			]
			onSelectionAccepted: {
				try {
					volume3dData.save(fileUrl);
				} catch (err) {
					console.log(err);
				}
			}
		}

		ComboBox {
			id: cmbPreview
			anchors.left: parent.left
			model: ['Thumb', 'Input', 'Backup', 'Output', 'Positions']
			onCurrentIndexChanged: switch (currentIndex) {
				case 0:
					volume3dView.display = Volume3dData.Thumb;
					break;
				case 1:
					volume3dView.display = Volume3dData.Input;
					break;
				case 2:
					volume3dView.display = Volume3dData.Backup;
					break;
				case 3:
					volume3dView.display = Volume3dData.Output;
					break;
				case 4:
					volume3dView.display = Volume3dData.Positions;
					break;
			}
		}
		TextField {
			id: openVolumePath
			anchors.left: cmbPreview.right
			anchors.right: btnOpen.left
			text: openVolumeDialog.fileUrl
			onAccepted: {
				var slices = +text;
				if (!isNaN(slices)) {
					text = openVolumeDialog.fileUrl;
					volume3dData.open(openVolumeDialog.fileUrl, slices);
				}
				else if (text == openVolumeDialog.fileUrl) {
					text = openVolumeDialog.fileUrl;
					volume3dData.open(openVolumeDialog.fileUrl);
				}
			}
		}
		Button {
			id: btnOpen
			anchors.right: btnSave.left
			text: 'Open..'
			onClicked: openVolumeDialog.open();
		}
		Button {
			id: btnSave
			text: 'Save..'
			anchors.right: parent.right
			menu: Menu {
				MenuItem {
					text: 'Backup'
					onTriggered: volume3dData.backup()
				}
				MenuItem {
					text: 'Volume'
					onTriggered: saveVolumeDialog.open()
				}
				MenuItem {
					text: 'Layout Settings'
					onTriggered: saveSettings('layout')
				}
				MenuItem {
					text: 'View Settings'
					onTriggered: saveSettings('view')
				}
				MenuItem {
					text: 'All Settings'
					onTriggered: saveSettings()
				}
			}
		}
	}

	TabView {
		id: tabs
		anchors {
			top: volume.bottom
			left: parent.left
			right: parent.right
			bottom: logs.visible ? logs.top : parent.bottom
		}
		Tab {
			title: 'Preview'
			anchors {
				fill: parent
				margins: 12
			}
			Column {
				anchors.fill: parent
				ColorRow {
					text: 'Background'
					textWidth: labelWidth
					spacing: root.spacing
					value: volume3dView.backgroundColor || '#000'
					onValueUpdated: volume3dView.backgroundColor = value;
				}
				SliderRow {
					text: 'Rotate x'
					textWidth: labelWidth
					spacing: root.spacing
					value: volume3dView.rotx
					precision: 0
					maximumValue: 360
					onValueUpdated: volume3dView.rotx = value
				}
				SliderRow {
					text: 'Rotate y'
					textWidth: labelWidth
					spacing: root.spacing
					value: volume3dView.roty
					precision: 0
					maximumValue: 360
					onValueUpdated: volume3dView.roty = value
				}
				SliderRow {
					text: 'Rotate z'
					textWidth: labelWidth
					spacing: root.spacing
					value: volume3dView.rotz
					precision: 0
					maximumValue: 360
					onValueUpdated: volume3dView.rotz = value
				}
				SliderRow {
					text: 'Plane'
					textWidth: labelWidth
					spacing: root.spacing
					value: volume3dView.plane
					minimumValue: -1
					maximumValue: 0
					onValueUpdated: volume3dView.plane = value;
				}
				SliderRow {
					text: 'Zoom'
					textWidth: labelWidth
					spacing: root.spacing
					value: volume3dView.zoom
					precision: 2
					minimumValue: 1
					maximumValue: 64
					onValueUpdated: volume3dView.zoom = value;
				}
				SliderRow {
					text: 'Alpha'
					textWidth: labelWidth
					spacing: root.spacing
					value: volume3dView.alpha
					minimumValue: 0
					maximumValue: 1
					onValueUpdated: volume3dView.alpha = value;
					updateOnRelease: volume3dView.display != Volume3dData.Thumb
				}
				SliderRow {
					text: 'Threshold'
					textWidth: labelWidth
					spacing: root.spacing
					value: volume3dView.threshold < 0 ? -volume3dView.threshold : volume3dView.threshold
					minimumValue: 0
					maximumValue: 1
					checked: true
					checkable: true
					onValueUpdated: if (volume3dView.threshold !== checked ? value : -value) {
						volume3dView.threshold = checked ? value : -value;
						volume3dView.preview(pressed ? Volume3dData.Thumb : volume3dView.display);
					}
					onPressedChanged: if (!pressed) {
						volume3dView.preview(volume3dView.display);
					}
					//onValueUpdated: volume3dView.threshold = checked ? value : -value;
					//updateOnRelease: volume3dView.display != Volume3dData.Thumb
				}
				SliderRow {
					text: 'Brightness'
					textWidth: labelWidth
					spacing: root.spacing
					value: volume3dView.brightness
					minimumValue: -1
					maximumValue: +1
					onValueUpdated: if (volume3dView.brightness !== value) {
						volume3dView.brightness = value;
						volume3dView.preview(pressed ? Volume3dData.Thumb : volume3dView.display);
					}
					onPressedChanged: if (!pressed) {
						volume3dView.preview(volume3dView.display);
					}
				}
				SliderRow {
					text: 'Contrast'
					textWidth: labelWidth
					spacing: root.spacing
					value: volume3dView.contrast
					minimumValue: -1
					maximumValue: +5
					onValueUpdated: if (volume3dView.contrast !== value) {
						volume3dView.contrast = value;
						volume3dView.preview(pressed ? Volume3dData.Thumb : volume3dView.display);
					}
					onPressedChanged: if (!pressed) {
						volume3dView.preview(volume3dView.display);
					}
				}
				SliderRow {
					text: 'Gamma'
					textWidth: labelWidth
					spacing: root.spacing
					value: volume3dView.gamma
					minimumValue: 0
					maximumValue: 5
					onValueUpdated: if (volume3dView.gamma !== value) {
						volume3dView.gamma = value;
						volume3dView.preview(pressed ? Volume3dData.Thumb : volume3dView.display);
					}
					onPressedChanged: if (!pressed) {
						volume3dView.preview(volume3dView.display);
					}
				}
				ColorRow {
					text: 'Highlight'
					textWidth: labelWidth
					spacing: root.spacing
					value: volume3dView.highlightColor || '#000'
					onValueUpdated: volume3dView.highlightColor = value;
				}
				SliderRow {
					text: 'Highlight'
					textWidth: labelWidth
					spacing: root.spacing
					value: Math.abs(root.selR)
					minimumValue: .002
					maximumValue: .700
					checked: root.selR > 0
					checkable: true
					onValueUpdated: {
						value = checked ? value : -value;
						if (root.selR !== value) {
							root.selR = Math.abs(value);
							volume3dView.preview(pressed ? Volume3dData.Thumb : volume3dView.display);
							root.selR = value;
						}
					}
					onPressedChanged: if (!pressed) {
						volume3dView.preview(volume3dView.display);
					}
				}
			}
		}
		Tab {
			title: 'Operations'
			anchors {
				fill: parent
				margins: 12
			}
			Column {
				anchors.fill: parent
				FileDialog {
					id: saveOperationsFile
					selectExisting: false
					selectMultiple: false
					nameFilters: [
						'Json files (*.json)',
						'All files (*)'
					]
					onSelectionAccepted: {
						try {
							var ops = [];
							for(var i = 0; i < operations.count; ++i) {
								var op = operations.get(i);
								var newOp = {};
								for (var k in op) {
									if (k === 'enabled') {
										continue;
									}
									newOp[k] = op[k];
								}
								if (op.enabled === false) {
									newOp.name = '-' + op.name;
								}
								ops.push(newOp);
							}
							var request = new XMLHttpRequest();
							request.open("PUT", fileUrl, false);
							request.send(JSON.stringify(ops, null, '  '));
						} catch (err) {
							console.log(err);
						}
					}
				}

				FileDialog {
					id: openOperationsFile
					selectExisting: true
					selectMultiple: false
					nameFilters: [
						'Json files (*.json)',
						'All files (*)'
					]
					onSelectionAccepted: {
						try {
							var request = new XMLHttpRequest();
							request.open("GET", fileUrl, false);
							request.send(null);
							var ops = JSON.parse(request.responseText);

							operations.clear();
							for(var i in ops) {
								var op = ops[i];
								if (op.name !== undefined) {
									if (op.name.charAt(0) === '-') {
										op.name = op.name.substr(1);
										op.enabled = false;
									} else {
										op.enabled = true;
									}
								}
								operations.append(op);
							}
						} catch (err) {
							console.log(err);
						}
					}
				}

				Row {
					id: operationRow
					Button {
						text: 'Operations'
						menu: Menu {
							MenuItem {
								text: 'Import'
								onTriggered: openOperationsFile.open()
							}
							MenuItem {
								text: 'Export'
								onTriggered: saveOperationsFile.open()
							}
							MenuItem {
								text: 'Clear'
								onTriggered: operations.clear()
							}
						}
					}
					Button {
						text: 'Add'
						menu: Menu {
							MenuItem {
								text: 'RestoreView'
								onTriggered: {
									var arr = [];
									var list = volume3dView.getTransform();
									for(var i = 0; i < list.length; ++i) {
										arr.push(list[i]);
									}
									operations.append({
										name: text, enabled: true,
										transform: arr,
										cutPlane: volume3dView.plane,
										magnify: volume3dView.zoom,
										alpha: volume3dView.alpha,
										threshold: volume3dView.threshold,
										brightness: volume3dView.brightness,
										contrast: volume3dView.contrast,
										gamma: volume3dView.gamma,
									});
								}
							}
							MenuItem {
								text: 'CropSphere'
								onTriggered: operations.append({
									name: text, enabled: true,
									x: selX, y: selY, z: selZ, r: selR,
								});
							}
							MenuItem {
								text: 'CutSphere'
								onTriggered: operations.append({
									name: text, enabled: true,
									x: selX, y: selY, z: selZ, r: selR,
								});
							}
							MenuItem {
								text: 'Threshold'
								onTriggered: operations.append({
									name: text, enabled: true,
									min: 0, max: 1, norm: false
								});
							}
							MenuItem {
								text: 'Clahe'
								onTriggered: operations.append({
									name: text, enabled: true,
									windowSize: 127, bins: 256, clipLimit: 3
								});
							}
							MenuItem {
								text: 'BoxBlur'
								onTriggered: operations.append({
									name: text, enabled: true,
									kernel: Volume3dData.Box, size: 3,
								});
							}
							MenuItem {
								text: 'GaussBlur'
								onTriggered: operations.append({
									name: text, enabled: true,
									kernel: Volume3dData.Gauss, size: 3,
								});
							}
							MenuItem {
								text: 'Median'
								onTriggered: operations.append({
									name: text, enabled: true,
									kernel: Volume3dData.Box, size: 3,
								});
							}
							MenuItem {
								text: 'Erode'
								onTriggered: operations.append({
									name: text, enabled: true,
									kernel: Volume3dData.Box, size: 3,
								});
							}
							MenuItem {
								text: 'Dilate'
								onTriggered: operations.append({
									name: text, enabled: true,
									kernel: Volume3dData.Box, size: 3,
								});
							}
							Menu {
								title: 'CustomFilter'
								MenuItem {
									text: 'Normal'
									onTriggered: operations.append({
										name: 'CustomFilter', enabled: true,
										size: 3, values: [
											0, 0, 0,
											0, 0, 0,
											0, 0, 0,

											0, 0, 0,
											0, 1, 0,
											0, 0, 0,

											0, 0, 0,
											0, 0, 0,
											0, 0, 0,
										],
									});
								}
								MenuItem {
									text: 'Sharpen More'
									onTriggered: operations.append({
										name: 'CustomFilter', enabled: true,
										size: 3, values: [
											0,  0,  0,
											0, -1,  0,
											0,  0,  0,

											0, -1,  0,
											-1, 7, -1,
											0, -1,  0,

											0,  0,  0,
											0, -1,  0,
											0,  0,  0
										],
									});
								}
								MenuItem {
									text: 'Sharpen Hard'
									onTriggered: operations.append({
										name: 'CustomFilter', enabled: true,
										size: 3, values: [
											 0, -1,  0,
											-1, -1, -1,
											 0, -1,  0,

											-1, -1, -1,
											-1, 19, -1,
											-1, -1, -1,

											 0, -1,  0,
											-1, -1, -1,
											 0, -1,  0
										],
									});
								}
							}
						}
					}
					Button {
						text: 'Apply'
						onClicked: {
							for(var i = 0; i < operations.count; ++i) {
								var op = operations.get(i);
								if (op.enabled) {
									operations.apply[op.name](op);
								}
							}
						}
					}
					Button {
						text: 'Restore'
						onClicked: volume3dData.restore();
					}
				}
				OperationList {
					id: operations

					width: parent.width
					height: parent.height - operationRow.height
					contentHeight: height
					labelWidth: root.labelWidth
					spacing: root.spacing
					updateOnRelease: false//volume3dView.display != Volume3dData.Thumb

					onValueChanged: if (index >= 0) {
						var display = Volume3dData.Thumb;
						if (updateOnRelease || field === 'Preview') {
							display = volume3dView.display;
						}
						//operationLog.d('ValueChanged[' + index + '].' + field + ': ' + JSON.stringify(item));
						operations.preview[item.name](display, item, field);
					}

					property var apply: {
						'CropSphere': function(op) {
							volume3dData.cutCropSphere(op.x, op.y, op.z, op.r, true)
						},
						'CutSphere': function(op) {
							volume3dData.cutCropSphere(op.x, op.y, op.z, op.r, false)
						},
						'Threshold': function(op) {
							volume3dData.threshold(op.min, op.max, op.norm)
						},
						'BoxBlur': function(op) {
							volume3dData.filter(Volume3dData.Filter, op.kernel, op.size, 1 / (op.size * op.size * op.size));
						},
						'GaussBlur': function(op) {
							volume3dData.filter(Volume3dData.Filter, op.kernel, op.size, op.size / 4);
						},
						'CustomFilter': function(op) {
							volume3dData.filter(op.size, op.values);
						},
						'Erode': function(op) {
							volume3dData.filter(Volume3dData.Erode, op.kernel, op.size, 1);
						},
						'Dilate': function(op) {
							volume3dData.filter(Volume3dData.Dilate, op.kernel, op.size, 1);
						},
						'Median': function(op) {
							volume3dData.filter(Volume3dData.Median, op.kernel, op.size, 1);
						},
						'Clahe': function(op) {
							volume3dData.clahe(op.bins, op.windowSize, op.clipLimit);
						},
						'RestoreView': function(op) {
							volume3dView.plane = op.cutPlane;
							volume3dView.zoom = op.magnify;
							volume3dView.alpha = op.alpha;
							volume3dView.threshold = op.threshold;
							volume3dView.brightness = op.brightness;
							volume3dView.contrast = op.contrast;
							volume3dView.gamma = op.gamma;
							volume3dView.setTransform(op.transform);
							volume3dView.preview();
						}
					}

					property var preview: {
						'CropSphere': function(quality, op) {
							volume3dView.render(quality, op.x, op.y, op.z, op.r);
						},
						'CutSphere': function(quality, op) {
							volume3dView.render(quality, op.x, op.y, op.z, op.r);
						},
						'Threshold': function(quality, op, field) {
							if (field === 'min' || field === 'max') {
								volume3dView.render(quality, op[field]);
								return;
							}
							volume3dView.render(quality);
						},
						'BoxBlur': function(quality, op) { volume3dView.preview(Volume3dData.Thumb, false); },
						'GaussBlur': function(quality, op) { volume3dView.preview(Volume3dData.Thumb, false); },
						'CustomFilter': function(quality, op) { volume3dView.preview(Volume3dData.Thumb, false); },
						'Erode': function(quality, op) { volume3dView.preview(Volume3dData.Thumb, false); },
						'Dilate': function(quality, op) { volume3dView.preview(Volume3dData.Thumb, false); },
						'Median': function(quality, op) { volume3dView.preview(Volume3dData.Thumb, false); },
						'Clahe': function(quality, op) { /* no preview */ },
						'RestoreView': function(quality, op, field) {
							if (field === 'enabled') {
								return;
							}
							operations.apply[op.name](op);
						}
					}
				}
			}
		}
		Tab {
			title: 'Logs'
			anchors {
				fill: parent
				margins: 12
			}

			StackView {
				ListView {
					anchors.fill: parent
					model: operationLog
					delegate: Text {
						function toSeconds(time, pad) {
							var result = (time / 1000).toFixed(2);
							while (result.length < pad + 3) {
								result = '0' + result;
							}
							return result;
						}
						color: model.color || "black"
						text: '[@' + toSeconds(model.time, 4) + ']: ' + model.text + (model.elapsed < 0 ? '' : ' completed in: ' + toSeconds(model.elapsed) + ' seconds')
					}
				}

				Button {
					text: 'Clear'
					anchors.top: parent.top;
					anchors.right: parent.right;
					onClicked: operationLog.clear();
				}
			}
		}
	}

	StackView {
		id: logs
		visible: tabs.currentIndex != 2
		height: parent.height / 3
		clip: true
		anchors {
			left: parent.left
			right: parent.right
			bottom: parent.bottom
		}

		ListView {
			anchors.fill: parent
			model: operationLog
			delegate: Text {
				color: model.color || "black"
				text: operationLog.asString(model);
			}
		}

		Button {
			text: 'Clear'
			anchors.top: parent.top;
			anchors.right: parent.right;
			onClicked: operationLog.clear();
		}
	}

	Component.onCompleted: {
		operationLog.i('configuration: ' + settings.path());
		operationLog.i('volume resolution is: ' + volumeResolution);
		volume3dView.visible = true;
	}
}
