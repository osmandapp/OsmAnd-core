package net.osmand.core.samples.android.sample1;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.support.annotation.ColorInt;
import android.support.annotation.DrawableRes;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

import net.osmand.core.jni.Amenity;
import net.osmand.core.jni.LatLon;
import net.osmand.core.jni.Utilities;

public class SearchUIHelper {

	public static abstract class SearchListItem {

		public SearchListItem() {
		}

		public abstract Drawable getIcon();

		public abstract String getName();

		public abstract double getLatitude();

		public abstract double getLongitude();

		@Override
		public String toString() {
			return getName() + " {lat:" + getLatitude() + " lon: " + getLongitude() + "}";
		}
	}

	public static class AmenityListItem extends SearchListItem {

		private String name;
		private double latitude;
		private double longitude;

		public AmenityListItem(Amenity amenity) {
			name = amenity.getNativeName();
			LatLon latLon = Utilities.convert31ToLatLon(amenity.getPosition31());
			latitude = latLon.getLatitude();
			longitude = latLon.getLongitude();
		}

		@Override
		public Drawable getIcon() {
			return null;
		}

		@Override
		public String getName() {
			return name;
		}

		@Override
		public double getLatitude() {
			return latitude;
		}

		@Override
		public double getLongitude() {
			return longitude;
		}
	}

	public static class SearchListAdapter extends ArrayAdapter<SearchListItem> {

		private Context ctx;

		public SearchListAdapter(Context ctx) {
			super(ctx, android.R.layout.simple_list_item_1);
			this.ctx = ctx;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			SearchListItem item = getItem(position);
			TextView view = (TextView) super.getView(position, convertView, parent);
			view.setText(item.getName());
			view.setTextColor(ctx.getResources().getColor(R.color.listTextColor));
			view.setCompoundDrawablesWithIntrinsicBounds(item.getIcon(), null, null, null);
			view.setCompoundDrawablePadding(ctx.getResources().getDimensionPixelSize(R.dimen.list_content_padding));
			return view;
		}
	}
}
