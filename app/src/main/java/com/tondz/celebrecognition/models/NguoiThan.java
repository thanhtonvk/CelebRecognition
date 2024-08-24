package com.tondz.celebrecognition.models;

public class NguoiThan {
    private String ten;
    private byte[] anh;
    private String embedding;

    public String getEmbedding() {
        return embedding;
    }

    public void setEmbedding(String embedding) {
        this.embedding = embedding;
    }

    public String getTen() {
        return ten;
    }

    public void setTen(String ten) {
        this.ten = ten;
    }

    public byte[] getAnh() {
        return anh;
    }

    public void setAnh(byte[] anh) {
        this.anh = anh;
    }

    public NguoiThan(String ten, byte[] anh, String embedding) {
        this.ten = ten;
        this.anh = anh;
        this.embedding = embedding;
    }
}
