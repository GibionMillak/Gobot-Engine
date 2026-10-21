export default {
	multipass: true,
	precision: 2,
	js2svg: {
		pretty: true,
		indent: "\t",
		eol: "lf",
		finalNewline: true,
	},
	plugins: [
		{
			name: "ensure_svg_attributes",
			fn: () => ({
				element: {
					enter: (p_node) => {
						if (p_node.name !== "svg") {
							return;
						}
						p_node.attributes.xmlns = "http://www.w3.org/2000/svg";
						if (p_node.attributes.viewBox !== undefined) {
							return;
						}
						const width = p_node.attributes.width?.match(/^([\d.]+)(?:px)?$/);
						const height = p_node.attributes.height?.match(/^([\d.]+)(?:px)?$/);
						if (!width || !height || Number(width[1]) <= 0 || Number(height[1]) <= 0) {
							throw new Error("Cannot derive SVG viewBox without positive numeric dimensions.");
						}
						p_node.attributes.viewBox = `0 0 ${width[1]} ${height[1]}`;
					},
				},
			}),
		},
		{
			name: "remove_unused_space_preservation",
			fn: (p_root) => {
				const has_text = (p_node) => ["text", "tspan", "textPath"].includes(p_node.name) || p_node.children?.some(has_text);
				if (has_text(p_root)) {
					return;
				}
				return {
					element: {
						enter: (p_node) => {
							delete p_node.attributes["xml:space"];
						},
					},
				};
			},
		},
		{
			name: "preset-default",
			params: {
				overrides: {
					removeHiddenElems: false,
					convertPathData: false,
					sortAttrs: false,
				},
			},
		},
		"convertStyleToAttrs",
		"removeScripts",
		"removeStyleElement",
		"reusePaths",
		{
			name: "removeAttrs",
			params: { attrs: "class" },
		},
		{
			name: "remove_icon_editor_attributes",
			fn: (p_root) => {
				const has_text = (p_node) => ["text", "tspan", "textPath"].includes(p_node.name) || p_node.children?.some(has_text);
				const has_current_color = (p_node) =>
					Object.values(p_node.attributes ?? {}).some((p_value) => /currentcolor/i.test(p_value)) ||
					p_node.children?.some(has_current_color);
				const uses_current_color = has_current_color(p_root);
				return {
					element: {
						enter: (p_node) => {
							p_node.children = p_node.children.filter((p_child) =>
								p_child.type !== "element" || p_child.attributes.display !== "none");
							if (!uses_current_color) {
								delete p_node.attributes.color;
							}
							if (p_node.attributes.style !== undefined) {
								const uses_text = has_text(p_node);
								const declarations = p_node.attributes.style.split(";").filter((p_declaration) => {
									const [name, value] = p_declaration.split(":").map((p_part) => p_part.trim());
									if (name.startsWith("-inkscape-") ||
										(!uses_text && /^(font-|text-|white-space|inline-size|shape-margin|shape-padding)/.test(name))) {
										return false;
									}
									return !(["isolation", "mix-blend-mode"].includes(name) && ["auto", "normal"].includes(value));
								});
								if (declarations.length === 0) {
									delete p_node.attributes.style;
								} else {
									p_node.attributes.style = declarations.join(";");
								}
							}
						},
					},
				};
			},
		},
		{
			name: "normalize_icon_paint",
			fn: () => ({
				element: {
					exit: (p_node) => {
						if (p_node.name !== "svg") {
							return;
						}
						const elements = [];
						const collect_elements = (p_element) => {
							if (p_element.type === "element") {
								elements.push(p_element);
							}
							p_element.children?.forEach(collect_elements);
						};
						collect_elements(p_node);
						// References, CSS, and animation can change paint outside this inheritance tree.
						const dynamic_elements = ["use", "style", "foreignObject", "animate", "animateTransform", "set"];
						if (elements.some((p_element) =>
							dynamic_elements.includes(p_element.name) || p_element.attributes.style !== undefined)) {
							return;
						}
						const paint_elements = new Set([
							"path", "rect", "circle", "ellipse", "polygon", "polyline", "line", "text", "tspan", "textPath",
						]);
						const fills = new Set();
						const collect_fills = (p_element, p_inherited_fill) => {
							const declared_fill = p_element.attributes?.fill;
							const fill = declared_fill === undefined || declared_fill === "inherit" ?
								p_inherited_fill : declared_fill;
							if (paint_elements.has(p_element.name)) {
								fills.add(["black", "#000000"].includes(fill) ? "#000" : fill);
							}
							p_element.children?.forEach((p_child) => collect_fills(p_child, fill));
						};
						collect_fills(p_node, "#000");
						if (fills.size === 1) {
							const [fill] = fills;
							if (!fill.startsWith("url(")) {
								for (const element of elements) {
									delete element.attributes.fill;
								}
								p_node.attributes.fill = fill;
							}
						}
						const has_stroke = elements.some((p_element) =>
							p_element.attributes.stroke !== undefined && p_element.attributes.stroke !== "none");
						const has_marker = elements.some((p_element) =>
							Object.keys(p_element.attributes).some((p_name) => p_name.startsWith("marker")));
						if (!has_stroke && !has_marker) {
							for (const element of elements) {
								delete element.attributes["paint-order"];
								for (const name of Object.keys(element.attributes)) {
									if (name === "stroke" || name.startsWith("stroke-")) {
										delete element.attributes[name];
									}
								}
							}
						}
					},
				},
			}),
		},
		{
			name: "order_icon_attributes",
			fn: () => {
				const order = [
					"xmlns", "xmlns:xlink", "id", "href", "xlink:href", "transform", "gradientUnits", "gradientTransform",
					"in", "in2", "result", "operator", "values", "xml:space",
					"width", "height", "viewBox", "preserveAspectRatio", "x", "y", "x1", "y1", "x2", "y2", "cx", "cy",
					"r", "rx", "ry", "dx", "dy", "d", "points", "offset", "stdDeviation",
					"fill", "fill-rule", "fill-opacity", "color", "opacity", "display", "clip-path", "clip-rule", "mask",
					"filter", "color-interpolation-filters", "stop-color", "stop-opacity", "flood-color", "flood-opacity",
					"font-family", "font-size", "font-weight", "letter-spacing", "word-spacing", "paint-order", "style",
					"stroke", "stroke-width", "stroke-linecap", "stroke-linejoin", "stroke-miterlimit", "stroke-dasharray",
					"stroke-dashoffset", "stroke-opacity",
				];
				return {
					element: {
						enter: (p_node) => {
							const attributes = Object.entries(p_node.attributes);
							attributes.sort(([p_left], [p_right]) => {
								const left_index = order.indexOf(p_left);
								const right_index = order.indexOf(p_right);
								return (left_index === -1 ? order.length : left_index) -
									(right_index === -1 ? order.length : right_index) || p_left.localeCompare(p_right);
							});
							p_node.attributes = Object.fromEntries(attributes);
						},
					},
				};
			},
		},
	],
};
