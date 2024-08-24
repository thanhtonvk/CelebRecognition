package com.tondz.celebrecognition.database;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;

import androidx.annotation.Nullable;

import com.tondz.celebrecognition.models.NguoiThan;

import java.util.ArrayList;
import java.util.List;

public class DBContext extends SQLiteOpenHelper {
    public DBContext(@Nullable Context context) {
        super(context, "database.sqlite", null, 1);
    }

    @Override
    public void onCreate(SQLiteDatabase sqLiteDatabase) {
        sqLiteDatabase.execSQL("create table NguoiThan(" +
                "Ten nvarchar(50)," +
                "Anh blob," +
                "Embedding text)");
    }

    @Override
    public void onUpgrade(SQLiteDatabase sqLiteDatabase, int i, int i1) {

    }

    public List<NguoiThan> getNguoiThans() {
        List<NguoiThan> nguoiThans = new ArrayList<>();
        SQLiteDatabase database = getReadableDatabase();
        Cursor cursor = database.rawQuery("select * from NguoiThan", null);
        cursor.moveToFirst();
        while (!cursor.isAfterLast()) {
            NguoiThan nguoiThan = new NguoiThan(cursor.getString(0), cursor.getBlob(1), cursor.getString(2));
            nguoiThans.add(nguoiThan);
            cursor.moveToNext();
        }
        database.close();
        return nguoiThans;
    }

    public void add(NguoiThan nguoiThan) {
        SQLiteDatabase database = getWritableDatabase();
        ContentValues values = new ContentValues();
        values.put("Ten", nguoiThan.getTen());
        values.put("Anh", nguoiThan.getAnh());
        values.put("Embedding", nguoiThan.getEmbedding());
        database.insert("NguoiThan", null, values);
        database.close();
    }

    public void xoa(String embedding) {
        SQLiteDatabase database = getWritableDatabase();
        int result = database.delete("NguoiThan", "Embedding = ?", new String[]{embedding});
        Log.e("TAG Xoa", "xoa: "+result );
    }
}