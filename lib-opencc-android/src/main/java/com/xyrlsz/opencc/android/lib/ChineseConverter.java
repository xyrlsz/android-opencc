package com.xyrlsz.opencc.android.lib;

import android.content.Context;
import android.content.res.AssetManager;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Objects;
import java.util.concurrent.ConcurrentHashMap;

public class ChineseConverter {
    // 每种转换类型可设置独立的数据目录（库的扩展点），ConcurrentHashMap 保证线程安全
    private static final ConcurrentHashMap<ConversionType, String> dataFolderPathMap = new ConcurrentHashMap<>();
    private static volatile boolean initialized = false;

    static {
        System.loadLibrary("ChineseConverter");
    }

    private static boolean isEmptyString(String str) {
        return str == null || str.isEmpty();
    }

    /**
     * initialize the library
     *
     * @param context android context
     */
    public static void init(Context context) {
        if (!initialized) {
            synchronized (ChineseConverter.class) {
                if (!initialized) {
                    initialize(context);
                    initialized = true;
                }
            }
        }
    }

    /***
     * 为指定转换类型设置独立的数据目录路径。
     * 不调用则默认使用 init() 时设置的通用路径。
     *
     * @param type 转换类型
     * @param dataFolderPath 字典数据所在目录的绝对路径
     */
    public static void setDataFolderPath(ConversionType type, String dataFolderPath) {
        if (type == null || dataFolderPath == null) {
            throw new IllegalArgumentException("type and dataFolderPath must not be null");
        }
        dataFolderPathMap.put(type, dataFolderPath);
    }

    private static String getDataFolderPathForType(ConversionType conversionType) {
        String path = dataFolderPathMap.get(conversionType);
        if (path == null) {
            // 特定类型未单独设置，回退到 S2T 默认路径
            path = dataFolderPathMap.get(ConversionType.S2T);
            if (path == null) {
                throw new RuntimeException("No data folder path initialized for any conversion type");
            }
        }
        return path;
    }

    /***
     * 转换文本。需要先调用 {@link #init(Context)} 初始化。
     *
     * @param text           待转换文本
     * @param conversionType 转换类型
     * @return 转换后的文本，空文本返回 ""
     * @throws RuntimeException 如果未初始化
     */
    public static String convert(String text, ConversionType conversionType) {
        if (isEmptyString(text)) {
            return "";
        }
        // 快速失败：给出明确的初始化提示
        if (!initialized) {
            throw new RuntimeException("Please call ChineseConverter.init(context) first.");
        }
        return nativeConvert(text, conversionType.getValue(),
                getDataFolderPathForType(conversionType));
    }

    /***
     * 转换文本。如未初始化则自动用给定 context 初始化。
     *
     * @param text           待转换文本
     * @param conversionType 转换类型
     * @param context        android context
     * @return 转换后的文本，空文本返回 ""
     */
    public static String convert(String text, ConversionType conversionType, Context context) {
        if (isEmptyString(text)) {
            return "";
        }
        if (!initialized) {
            synchronized (ChineseConverter.class) {
                if (!initialized) {
                    initialize(context);
                    initialized = true;
                }
            }
        }
        // 在 synchronized 块外获取路径：initialized 的 volatile 语义保证
        // ConcurrentHashMap 的写入对后续 volatile 读可见
        return nativeConvert(text, conversionType.getValue(),
                getDataFolderPathForType(conversionType));
    }

    /***
     * Clear the dictionary data folder, only call this method when update the dictionary data.
     * @param context android context
     */
    public static void clearDictDataFolder(Context context) {
        synchronized (ChineseConverter.class) {
            File dataFolder = new File(context.getFilesDir() + "/openccdata");
            deleteRecursive(dataFolder);
            dataFolderPathMap.clear();
            initialized = false;
        }
    }

    private static void deleteRecursive(File fileOrDirectory) {
        if (fileOrDirectory.isDirectory())
            for (File child : Objects.requireNonNull(fileOrDirectory.listFiles()))
                deleteRecursive(child);

        fileOrDirectory.delete();
    }

    private static native String nativeConvert(String text, String configFile, String absoluteDataFolderPath);

    private static void initialize(Context context) {
        File baseDir = context.getFilesDir();
        File dataDir = new File(baseDir, "openccdata");

        if (!dataDir.exists()) {
            copyFolder("openccdata", context);
        }

        // 为每种转换类型设置默认数据路径（不覆盖已通过 setDataFolderPath 自定义的路径）
        String defaultPath = dataDir.getAbsolutePath();
        for (ConversionType type : ConversionType.values()) {
            dataFolderPathMap.putIfAbsent(type, defaultPath);
        }
    }

    private static void copyFolder(String folderName, Context context) {
        File fileFolderOnDisk = new File(context.getFilesDir() + "/" + folderName);
        AssetManager assetManager = context.getAssets();
        String[] files = null;
        try {
            files = assetManager.list(folderName);
        } catch (IOException e) {
            Log.e("tag", "Failed to get asset file list.", e);
        }
        if (files != null) {
            for (String filename : files) {
                InputStream in = null;
                OutputStream out = null;
                try {
                    in = assetManager.open(folderName + "/" + filename);
                    if (!fileFolderOnDisk.exists()) {
                        fileFolderOnDisk.mkdirs();
                    }
                    File outFile = new File(fileFolderOnDisk.getAbsolutePath(), filename);
                    if (!outFile.exists()) {
                        outFile.createNewFile();
                    }
                    out = new FileOutputStream(outFile);
                    copyFile(in, out);
                } catch (IOException e) {
                    Log.e("tag", "Failed to copy asset file: " + filename, e);
                } finally {
                    if (in != null) {
                        try {
                            in.close();
                        } catch (IOException e) {
                            // NOOP
                        }
                    }
                    if (out != null) {
                        try {
                            out.close();
                        } catch (IOException e) {
                            // NOOP
                        }
                    }
                }
            }
        }
    }

    private static void copyFile(InputStream in, OutputStream out) throws IOException {
        byte[] buffer = new byte[8192];
        int read;
        while ((read = in.read(buffer)) != -1) {
            out.write(buffer, 0, read);
        }
    }
}
