package com.xyrlsz.opencc.android.lib;

import android.content.Context;
import android.content.res.AssetManager;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.HashMap;
import java.util.Map;

/**
 * Created by zhangqichuan on 29/2/16.
 */
public class ChineseConverter {
    private static volatile boolean initialized = false;
    private static Map<ConversionType, String> dataFolderPathMap = new HashMap<>();

    static {
        System.loadLibrary("ChineseConverter");
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

    private static String getDataFolderPathForType(ConversionType conversionType) {
        String path = dataFolderPathMap.get(conversionType);
        if (path == null) {
            // 如果特定类型的数据路径不存在，返回默认路径
            path = dataFolderPathMap.get(ConversionType.S2T); // 假设S2T为默认类型
            if (path == null) {
                throw new RuntimeException("No data folder path initialized for any conversion type");
            }
        }
        return path;
    }

    /***
     * @param text           the text to be converted to
     * @param conversionType the conversion type
     * @return the converted text
     */
    public static String convert(String text, ConversionType conversionType) {
        if (!initialized) {
            synchronized (ChineseConverter.class) {
                if (!initialized) {
                    throw new RuntimeException("Please call init() first.");
                }
            }
        }
        String specificDataPath = getDataFolderPathForType(conversionType);
        return convert(text, conversionType.getValue(), specificDataPath);
    }


    /***
     * @param text           the text to be converted to
     * @param conversionType the conversion type
     * @param context        android context
     * @return the converted text
     */
    public static String convert(String text, ConversionType conversionType, Context context) {
        if (!initialized) {
            synchronized (ChineseConverter.class) {
                if (!initialized) {
                    initialize(context);
                    initialized = true;
                }
            }
        }
        String specificDataPath = getDataFolderPathForType(conversionType);
        return convert(text, conversionType.getValue(), specificDataPath);
    }

    /***
     * Clear the dictionary data folder, only call this method when update the dictionary data.
     * @param context android context
     */
    public static void clearDictDataFolder(Context context) {
        File dataFolder = new File(context.getFilesDir() + "/openccdata");
        deleteRecursive(dataFolder);

        // 清空缓存的路径映射
        dataFolderPathMap.clear();
    }

    private static void deleteRecursive(File fileOrDirectory) {
        if (fileOrDirectory.isDirectory())
            for (File child : fileOrDirectory.listFiles())
                deleteRecursive(child);

        fileOrDirectory.delete();
    }

    private static native String convert(String text, String configFile, String absoluteDataFolderPath);

    private static void initialize(Context context) {
        File baseDir = context.getFilesDir();
        File dataDir = new File(baseDir, "openccdata");

        if (!dataDir.exists()) {
            copyFolder("openccdata", context);
        }

        // 为每个转换类型设置数据路径
        for (ConversionType type : ConversionType.values()) {
            dataFolderPathMap.put(type, dataDir.getAbsolutePath());
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
        byte[] buffer = new byte[1024];
        int read;
        while ((read = in.read(buffer)) != -1) {
            out.write(buffer, 0, read);
        }
    }
}
