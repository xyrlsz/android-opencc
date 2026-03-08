package com.xyrlsz.opencc.android.lib;

/**
 * Created by zhangqichuan on 2/3/16.
 */
public enum ConversionType {
    HK2S("hk2s.json"),
    HK2T("hk2t.json"),
    JP2T("jp2t.json"),
    S2HK("s2hk.json"),
    S2T("s2t.json"),
    S2TW("s2tw.json"),
    S2TWP("s2twp.json"),
    T2HK("t2hk.json"),
    T2S("t2s.json"),
    T2TW("t2tw.json"),
    T2JP("t2jp.json"),
    TW2S("tw2s.json"),
    TW2T("tw2t.json"),
    TW2SP("tw2sp.json");
    private final String value;

    ConversionType(String value) {
        this.value = value;
    }

    public String getValue() {
        return value;
    }
}
