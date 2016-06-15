package net.osmand.core.samples.android.sample1;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.LinearLayout;
import android.widget.TextView;

import net.osmand.core.samples.android.sample1.SearchAPI.SearchItem;

public class SearchUIHelper {

	public static Drawable getIcon(Context ctx, SearchItem item) {
		return null;
	}

	public static class SearchListAdapter extends ArrayAdapter<SearchItem> {

		private Context ctx;

		public SearchListAdapter(Context ctx) {
			super(ctx, R.layout.search_list_item);
			this.ctx = ctx;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			SearchItem item = getItem(position);

			LinearLayout view;
			if (convertView == null) {
				LayoutInflater inflater = (LayoutInflater) ctx
						.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
				view = (LinearLayout) inflater.inflate(
						R.layout.search_list_item, null);
			} else {
				view = (LinearLayout) convertView;
			}

			TextView text1 = (TextView) view.findViewById(R.id.text1);
			TextView text2 = (TextView) view.findViewById(R.id.text2);
			text1.setText(item.getLocalizedName());
			StringBuilder sb = new StringBuilder();
			if (!item.getSubTypeName().isEmpty()) {
				sb.append(getNiceString(item.getSubTypeName()));
			}
			if (!item.getTypeName().isEmpty()) {
				if (sb.length() > 0) {
					sb.append(" — ");
				}
				sb.append(getNiceString(item.getTypeName()));
			}
			text2.setText(sb.toString());
			//text1.setTextColor(ctx.getResources().getColor(R.color.listTextColor));
			//view.setCompoundDrawablesWithIntrinsicBounds(getIcon(ctx, item), null, null, null);
			//view.setCompoundDrawablePadding(ctx.getResources().getDimensionPixelSize(R.dimen.list_content_padding));
			return view;
		}
	}

	public static String capitalizeFirstLetterAndLowercase(String s) {
		if (s != null && s.length() > 1) {
			// not very efficient algorithm
			return Character.toUpperCase(s.charAt(0)) + s.substring(1).toLowerCase();
		} else {
			return s;
		}
	}

	public static String getNiceString(String s) {
		return capitalizeFirstLetterAndLowercase(s.replaceAll("_", " "));
	}
}
