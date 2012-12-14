package net.osmand.data;

import java.util.ArrayList;
import java.util.List;



public class QuadTree<T> {
	public static class QuadRect {
		public float left;
		public float right;
		public float top;
		public float bottom;
		public QuadRect(float left, float top, float right, float bottom) {
			this.left = left;
			this.right = right;
			this.top = top;
			this.bottom = bottom;
		}
		public QuadRect(QuadRect a) {
			this(a.left, a.top, a.right, a.bottom);
		}
		public float width() {
			return right - left;
		}
		public float height() {
			return bottom - top;
		}
	    public boolean contains(float left, float top, float right, float bottom) {
	        return this.left < this.right && this.top < this.bottom
	                && this.left <= left && this.top <= top
	                && this.right >= right && this.bottom >= bottom;
	    }
		public boolean contains(QuadRect box) {
			return contains(box.left, box.top, box.right, box.bottom);
		}
		public static boolean intersects(QuadRect a, QuadRect b) {
			return a.left < b.right && b.left < a.right
	                && a.top < b.bottom && b.top < a.bottom;
		}
		public float centerX() {
			return (left + right) / 2;
		}
		public float centerY() {
			return (top + bottom) / 2;
		}
		
		public void offset(float dx, float dy) {
			left    += dx;
	        top     += dy;
	        right   += dx;
	        bottom  += dy;
			
		}
		public void inset(float dx, float dy) {
			left    += dx;
	        top     += dy;
	        right   -= dx;
	        bottom  -= dy;
		}
		
		
	}
	
	private static class Node<T> {
		List<T> data = null;
		Node<T>[] children = null;
		QuadRect bounds;

		@SuppressWarnings("unchecked")
		private Node(QuadRect b) {
			bounds = new QuadRect(b.left, b.top, b.right, b.bottom);
			children = new Node[4];
		}
	};

	private float ratio;
	private int maxDepth;
	private Node<T> root;

	public QuadTree(QuadRect r, int depth/* =8 */, float ratio /* = 0.55 */) {
		this.ratio = ratio;
		this.root = new Node<T>(r);
		this.maxDepth = depth;
	}

	public void insert(T data, QuadRect box) {
		int depth = 0;
		doInsertData(data, box, root, depth);
	}
	
	public void insert(T data, float x, float y) {
		insert(data, new QuadRect(x, y, x, y));
	}

	public void queryInBox(QuadRect box, List<T> result) {
		result.clear();
		queryNode(box, result, root);
	}

	private void queryNode(QuadRect box, List<T> result, Node<T> node) {
		if (node != null) {
			if (QuadRect.intersects(box, node.bounds)) {
				if (node.data != null) {
					result.addAll(node.data);
				}
				for (int k = 0; k < 4; ++k) {
					queryNode(box, result, node.children[k]);
				}
			}
		}
	}

	private void doInsertData(T data, QuadRect box, Node<T> n, int depth) {
		if (++depth >= maxDepth) {
			if (n.data == null) {
				n.data = new ArrayList<T>();
			}
			n.data.add(data);
		} else {
			QuadRect[] ext = new QuadRect[4];
			splitBox(n.bounds, ext);
			for (int i = 0; i < 4; ++i) {
				if (ext[i].contains(box)) {
					if (n.children[i] == null) {
						n.children[i] = new Node<T>(ext[i]);
					}
					doInsertData(data, box, n.children[i], depth);
					return;
				}
			}
			if (n.data == null) {
				n.data = new ArrayList<T>();
			}
			n.data.add(data);
		}
	}

	void splitBox(QuadRect node_extent, QuadRect[] n) {
		// coord2d c=node_extent.center();

		float width = node_extent.width();
		float height = node_extent.height();

		float lox = node_extent.left;
		float loy = node_extent.top;
		float hix = node_extent.right;
		float hiy = node_extent.bottom;

		n[0] = new QuadRect(lox, loy, lox + width * ratio, loy + height * ratio);
		n[1] = new QuadRect(hix - width * ratio, loy, hix, loy + height * ratio);
		n[2] = new QuadRect(lox, hiy - height * ratio, lox + width * ratio, hiy);
		n[3] = new QuadRect(hix - width * ratio, hiy - height * ratio, hix, hiy);
	}

}
