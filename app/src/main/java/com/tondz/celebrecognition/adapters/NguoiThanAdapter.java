package com.tondz.celebrecognition.adapters;

import android.content.Context;
import android.graphics.Bitmap;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.tondz.celebrecognition.R;
import com.tondz.celebrecognition.database.DBContext;
import com.tondz.celebrecognition.models.NguoiThan;
import com.tondz.celebrecognition.utils.BitmapUtils;

import java.util.List;

public class NguoiThanAdapter extends RecyclerView.Adapter<NguoiThanAdapter.ViewHolder> {
    List<NguoiThan> nguoiThanList;
    Context context;
    DBContext dbContext;

    public NguoiThanAdapter(List<NguoiThan> nguoiThanList, Context context) {
        this.nguoiThanList = nguoiThanList;
        this.context = context;
        dbContext = new DBContext(context);
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        LayoutInflater inflater = LayoutInflater.from(context);
        View heroView = inflater.inflate(R.layout.item_nguoithan, parent, false);
        ViewHolder viewHolder = new ViewHolder(heroView);
        return viewHolder;
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        NguoiThan nguoiThan = nguoiThanList.get(position);
        Bitmap bitmap = BitmapUtils.getImage(nguoiThan.getAnh());
        holder.imgAvatar.setImageBitmap(bitmap);
        holder.tvName.setText(nguoiThan.getTen());
        holder.btnXoa.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dbContext.xoa(nguoiThan.getEmbedding());
                nguoiThanList.remove(nguoiThan);
                notifyDataSetChanged();
            }
        });
    }

    @Override
    public int getItemCount() {
        return nguoiThanList.size();
    }

    class ViewHolder extends RecyclerView.ViewHolder {
        ImageView imgAvatar;
        TextView tvName;
        Button btnXoa;

        public ViewHolder(@NonNull View itemView) {
            super(itemView);
            imgAvatar = itemView.findViewById(R.id.imageAvatar);
            tvName = itemView.findViewById(R.id.tvTen);
            btnXoa = itemView.findViewById(R.id.btnXoa);
        }
    }
}
