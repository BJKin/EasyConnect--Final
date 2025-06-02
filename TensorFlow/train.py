import numpy as np
import pandas as pd
import os
from sklearn.model_selection import train_test_split
from sklearn.metrics import confusion_matrix
import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense, Flatten, Dropout, Conv1D, MaxPooling1D, Input
from tensorflow.keras.utils import to_categorical
import matplotlib.pyplot as plt
import seaborn as sns
from datetime import datetime

np.random.seed(42)
tf.random.set_seed(42)

def loadCSV(filepath):
    try:
        dataframe = pd.read_csv(filepath)
        data = dataframe.values
        return data
    except Exception as e:
        print(f"Error loading {filepath}: {e}")
        return

def createSlidingWindows(data, window_size, step_size=1):
    num_windows = (len(data) - window_size) // step_size + 1
    num_features = data.shape[1]
    windows = np.zeros((num_windows, window_size, num_features))

    for i in range(num_windows):
        start_idx = i * step_size
        end_idx = start_idx + window_size
        windows[i] = data[start_idx:end_idx]
    
    return windows

def loadData(base_directory, window_size, step_size, feature_columns=None):
    all_windows = []
    all_labels = []
    class_names = []

    for gesture_name in sorted(os.listdir(base_directory)):
        gesture_path = os.path.join(base_directory, gesture_name)
        class_names.append(gesture_name)
        class_label = len(class_names) - 1 
        gesture_windows = []
        
        for csv_file in os.listdir(gesture_path):  
            csv_path = os.path.join(gesture_path, csv_file)
            sensor_data = loadCSV(csv_path)
                
            if feature_columns is not None:
                df_temp = pd.read_csv(csv_path)
                sensor_data = df_temp[feature_columns].values
            
            windows = createSlidingWindows(sensor_data, window_size, step_size)
            gesture_windows.append(windows)
        
        if gesture_windows:
            gesture_windows = np.vstack(gesture_windows)
            all_windows.append(gesture_windows)
            gesture_labels = np.full(len(gesture_windows), class_label)
            all_labels.append(gesture_labels)
    
    if all_windows:
        X = np.vstack(all_windows)
        y = np.concatenate(all_labels)
        
        return X, y, class_names

def buildModel(timesteps, features, classes):
    model = Sequential([
        Input(shape=(timesteps, features)),
        Conv1D(filters=64, kernel_size=4, activation='relu'),
        Conv1D(filters=64, kernel_size=4, activation='relu'),
        Dropout(0.5),
        MaxPooling1D(pool_size=2),
        Flatten(),
        Dense(100, activation='relu'),
        Dense(classes, activation='softmax')
    ])
    
    return model

def trainModel(X, y, class_names, epochs, batch_size):
    y_categorical = to_categorical(y)
    n_classes = len(class_names)
    
    X_train, X_test, y_train, y_test = train_test_split(X, y_categorical, random_state=42, stratify=y)
    print(f"Training samples: {len(X_train)}")
    print(f"Testing samples: {len(X_test)}")
    
    n_timesteps, n_features = X.shape[1], X.shape[2]
    model = buildModel(n_timesteps, n_features, n_classes)
    model.compile(loss='categorical_crossentropy', optimizer='adam', metrics=['accuracy'])
    model.summary()
    
    history = model.fit(X_train, y_train, epochs=epochs, batch_size=batch_size, validation_split=0.2, verbose=1, shuffle=True)
    return model, history, X_test, y_test
    
def testModel(model, history, X_test, y_test, class_names, timestamp):
    test_loss, test_accuracy = model.evaluate(X_test, y_test, verbose=0)
    print(f"Test Accuracy: {test_accuracy:.4f}")
    print(f"Test Loss: {test_loss:.4f}")
    
    y_pred = model.predict(X_test)
    y_pred_classes = np.argmax(y_pred, axis=1)
    y_test_classes = np.argmax(y_test, axis=1)
    
    cm = confusion_matrix(y_test_classes, y_pred_classes)
    plt.figure(figsize=(8, 6))
    sns.heatmap(cm, annot=True, fmt='d', cmap='Blues', xticklabels=class_names, yticklabels=class_names)
    plt.title('Confusion Matrix')
    plt.xlabel('Predicted')
    plt.ylabel('Actual')
    plt.tight_layout()
    plt.savefig(f"./TensorFlow/ModelAnalysis/confusion_matrix_{timestamp}.png", dpi=300)
    plt.show()
    
    plt.figure(figsize=(12, 4))
    plt.plot(history.history['accuracy'], label='Training Accuracy')
    plt.plot(history.history['val_accuracy'], label='Validation Accuracy')
    plt.title('Model Accuracy')
    plt.xlabel('Epoch')
    plt.ylabel('Accuracy')
    plt.legend()
    plt.tight_layout()
    plt.savefig(f"./TensorFlow/ModelAnalysis/train_val_accuracy_{timestamp}.png", dpi=300)
    plt.show()

def exportModel(model, save_path):
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.representative_dataset = None 
    tflite_model = converter.convert()
    with open(save_path, 'wb') as f:
        f.write(tflite_model)

    return tflite_model

if __name__ == "__main__":    
    try:
        X, y, class_names = loadData("./TensorFlow/Data", 175, 1)
        model, history, X_test, y_test = trainModel(X, y, class_names, 100, 64)

        timestamp = datetime.now().strftime("%m%d_%H%M")
        testModel(model, history, X_test, y_test, class_names, timestamp)
        tflite_model = exportModel(model, f"./TensorFlow/Models/Handshake_{timestamp}.tflite")
        
    except Exception as e:
        print(f"Error during training: {e}")